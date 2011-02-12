// $Id: http_tester.cpp 91670 2010-09-08 18:02:26Z johnnyw $

#include "client.h"

int Client_Parameters::tcp_nodelay;
int Client_Parameters::sockbufsiz;
Stats *Client_Parameters::stats;

static void *
client_thread(void *data)
{
  Client_Parameters *cp = (Client_Parameters *) data;
  float latency = 0, throughput;
  URL *u = cp->url;

  // Check for presence of protocol, hostname and filename.

  if(!(u->get_protocol() && u->get_hostname() && u->get_filename())) {
    cerr << "Invalid URL" << endl;
    return 0;
  }

  cp->stats->i_have_started(cp->id);

  // Attempt connection
  connection webserver;

  if(webserver.connect(u->get_hostname(), cp->tcp_nodelay, cp->sockbufsiz)) return 0;
  // Send the request now.


  char request[BUFSIZ];

  ACE_Profile_Timer throughput_timer, latency_timer;
  throughput_timer.start();
  latency_timer.start();
  ACE_OS::sprintf(request,"GET /%s HTTP/1.0\r\n\r\n",u->get_filename());
  webserver.write_n(request, ACE_OS::strlen(request)) ;

  char buffer[BUFSIZ];
  ssize_t num_read = 0, total_read = 0;
  unsigned int first_time = 1;
  for(;;) {
    num_read = webserver.read(buffer, sizeof buffer);
    if(first_time) {
      ACE_Profile_Timer::ACE_Elapsed_Time et;
      latency_timer.stop();
      latency_timer.elapsed_time(et);
      latency =  et.real_time;
      first_time = 0;
    }
    if(num_read <= 0)
      break;
    total_read += num_read;
  }
  cp->stats->i_am_done(cp->id);
  ACE_Profile_Timer::ACE_Elapsed_Time et;
  throughput_timer.stop();
  throughput_timer.elapsed_time(et);
  throughput = (8 * total_read/et.real_time) / (1000 * 1000); //pow(10,6) ;
  cp->stats->log(cp->id, throughput, latency);
  webserver.close();
  return 0;
}

int driver(char *id, int total_num, float requests_sec, char *url1, float p1, char *url2, float p2, char *url3, float p3, int tcp_nodelay, int sockbufsiz) {

  // construct the client parameters packet

  Client_Parameters::tcp_nodelay = tcp_nodelay;
  Client_Parameters::sockbufsiz = sockbufsiz;

  Client_Parameters::stats = new Stats(total_num);

  int missed_deadlines = 0;
  // sleep_time is in microseconds, and requests_sec is per second, hence the pow(10,6)
  float sleep_time = (1/requests_sec) * (1000.0 * 1000.0); // pow(10,6);
  float delta = 0;
  ACE_OS::srand(ACE_OS::time(0));
  for(int i = 0; i < total_num; i++) { // i is used as a id for threads
    ACE_Profile_Timer timer;
    if(sleep_time < delta)
      {
        // cerr << "Requested rate is too high, sleep_time == " << sleep_time << ", and delta = " << delta << ", after " << i  << " iterations! " << endl;
        missed_deadlines++;
      }
    else
      {
        ACE_Time_Value tv(0, (long int) (sleep_time - delta));
        ACE_OS::sleep(tv);
        timer.start();
      }
    Client_Parameters *cp = new Client_Parameters(i);

    double r = ((double)ACE_OS::rand()/(double)RAND_MAX);
    // cerr << " choosing between " << url1 << url2 << url3 << " with r == " << r;
    if(r <= p1)   cp->url = new URL(url1);
    if( (r > p1) && (r <= (p1 + p2)))   cp->url = new URL(url2);
    if( (r > (p1 + p2)) && (r <= p1 + p2 + p3))  cp->url = new URL(url3);
    // cerr << "The URL being requested is " << cp->url->get_filename() << endl;


    (ACE_Thread_Manager::instance ())->spawn(client_thread, (void *) cp);
    timer.stop();
    ACE_Profile_Timer::ACE_Elapsed_Time et;
    timer.elapsed_time(et);
    delta = ( (0.4 * fabs(et.real_time * (1000 * 1000))) + (0.6 * delta) ); // pow(10,6)
  }

  // Join the other threads..
  (ACE_Thread_Manager::instance ())->wait();
  // Now output the data for this test
  cout << id;
  Client_Parameters::stats->output();
  cout << endl;
  if (missed_deadlines != 0)
    {
      cout << "missed deadlines " << missed_deadlines << endl;
      cout << "missed deadlines as a % of total requests: " << (float) missed_deadlines / total_num * 100 << endl;
    }
  return 0;
}


ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  // This will set the global scale factor if the ACE_SCALE_FACTOR
  // environment variable is set.
  ACE_High_Res_Timer::get_env_global_scale_factor ();

  if(argc < 3) {
    cerr << "Usage: " << argv[0] << " infile outfile " << endl;
    cerr << "The input file contains lines, with the following fields: " << endl;
    cerr << "experiment_id total_number_of_requests request_rate url1 p1 url2 p2 url3 p3 TCP_NODELAY SOCKET_RECV_BUFSIZ " << endl;

    return 1;
  }

  FILE *fp = ACE_OS::fopen(argv[1],"r");
  if(fp == 0) {
    ACE_POS::perror("fopen");
    return 2;
  }
  ACE_OS::close(1);
  int fd = ACE_OS::open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if(fd == -1) {
    ACE_OS::perror("open");
    return 3;
  }


  int total_num,  tcp, sock;
  char *id = new char[BUFSIZ];
  float rate, p1, p2, p3;
  char *url1 = new char[BUFSIZ];
  char *url2 = new char[BUFSIZ];
  char *url3 = new char[BUFSIZ];


  while(!feof(fp)) {
    fscanf(fp,"%s %d %f %s %f %s %f %s %f %d %d\n", id, &total_num, &rate, url1, &p1, url2, &p2, url3, &p3, &tcp, &sock);
    if (id[0] == '#') continue;
    ACE_OS::fprintf(stderr,"----\n");
    ACE_OS::fprintf(stderr,"\tNow performing experiment:%s\n\tSending %d requests at %f requests/second\n", id, total_num, rate);
    driver(id, total_num, rate, url1, p1, url2, p2, url3, p3, tcp, sock);
  }
  ACE_OS::fclose(fp);
  ACE_OS::close(fd);
  return 0;
}

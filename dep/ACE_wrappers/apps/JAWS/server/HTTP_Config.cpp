// $Id: HTTP_Config.cpp 91670 2010-09-08 18:02:26Z johnnyw $

// HTTP_Config.cpp

#include "ace/OS_NS_stdlib.h"
#include "HTTP_Config.h"

// static HTTP_Config_Info config_info;

HTTP_Config_Info *HTTP_Config::instance_ = 0;

HTTP_Config_Info *
HTTP_Config::instance (void)
{
  if (HTTP_Config::instance_ == 0)
    {
      HTTP_Config::instance_ = new HTTP_Config_Info;

      HTTP_Config::instance_->document_root (0);
      HTTP_Config::instance_->cgi_path (0);
      HTTP_Config::instance_->user_dir (0);
      HTTP_Config::instance_->dir_index (0);
      HTTP_Config::instance_->proxy_flag (0);
    }

  return HTTP_Config::instance_;
}

HTTP_Config_Info::HTTP_Config_Info (void)
  : document_root_ (0),
    cgi_path_ (0),
    user_dir_ (0),
    dir_index_ (0),
    proxy_flag_ (0)
{
}

HTTP_Config_Info::~HTTP_Config_Info (void)
{
}

const char *
HTTP_Config_Info::document_root (void) const
{
  return this->document_root_;
}

const char *
HTTP_Config_Info::cgi_path (void) const
{
  return this->cgi_path_;
}

const char *
HTTP_Config_Info::user_dir (void) const
{
  return this->user_dir_;
}

const char *
HTTP_Config_Info::dir_index (void) const
{
  return this->dir_index_;
}

int
HTTP_Config_Info::proxy_flag (void) const
{
  return this->proxy_flag_;
}

const char *
HTTP_Config_Info::document_root (const char *dr_string)
{
  if (dr_string)
    this->document_root_ = dr_string;
  else
    {
      this->document_root_ = ACE_OS::getenv ("JAWS_DOCUMENT_ROOT");
      if (!this->document_root_)
        this->document_root_ = ".";
    }

  return this->document_root_;
}

const char *
HTTP_Config_Info::cgi_path (const char *cp_string)
{
  if (cp_string)
    this->cgi_path_ = cp_string;
  else
    {
      this->cgi_path_ = ACE_OS::getenv ("JAWS_CGI_PATH");

      if (!this->cgi_path_)
        this->cgi_path_ = "cgi-bin";
    }

  return this->cgi_path_;
}

const char *
HTTP_Config_Info::user_dir (const char *ud_string)
{
  if (ud_string)
    this->user_dir_ = ud_string;
  else
    {
      this->user_dir_ = ACE_OS::getenv ("JAWS_USER_DIR");
      if (!this->user_dir_)
        this->user_dir_ = ".www";
    }

  return this->user_dir_;
}

const char *
HTTP_Config_Info::dir_index (const char *di_string)
{
  if (di_string)
    this->dir_index_ = di_string;
  else
    {
      this->dir_index_ = ACE_OS::getenv ("JAWS_DIR_INDEX");
      if (!this->dir_index_)
        this->dir_index_ = "index.html";
    }

  return this->dir_index_;
}

int
HTTP_Config_Info::proxy_flag (int pf)
{
  this->proxy_flag_ = pf;
  return this->proxy_flag_;
}

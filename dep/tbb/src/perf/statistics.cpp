/*
    Copyright 2005-2009 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks.

    Threading Building Blocks is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    version 2 as published by the Free Software Foundation.

    Threading Building Blocks is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Threading Building Blocks; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    As a special exception, you may use this file as part of a free software
    library without restriction.  Specifically, if other files instantiate
    templates or use macros or inline functions from this file, or you compile
    this file and link it with other files to produce an executable, this
    file does not by itself cause the resulting executable to be covered by
    the GNU General Public License.  This exception does not however
    invalidate any other reasons why the executable file might be covered by
    the GNU General Public License.
*/

#include "statistics.h"
#include "statistics_xml.h"

#define COUNT_PARAMETERS 3

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

void GetTime(char* buff,int size_buff) 
{
    tm *newtime;
    time_t timer;
    time(&timer);
    newtime=localtime(&timer);
    strftime(buff,size_buff,"%H:%M:%S",newtime); 
}

void GetDate(char* buff,int size_buff) 
{
    tm *newtime;
    time_t timer;
    time(&timer);  
    newtime=localtime(&timer);
    strftime(buff,size_buff,"%Y-%m-%d",newtime); 
}


StatisticsCollector::TestCase StatisticsCollector::SetTestCase(const char *name, const char *mode, int threads)
{
    string KeyName(name);
    switch (SortMode)
    {
    case ByThreads: KeyName += Format("_%02d_%s", threads, mode); break;
    default:
    case ByAlg: KeyName += Format("_%s_%02d", mode, threads); break;
    }
    CurrentKey = Statistics[KeyName];
    if(!CurrentKey) {
        CurrentKey = new StatisticResults;
        CurrentKey->Mode = mode;
        CurrentKey->Name = name;
        CurrentKey->Threads = threads;
        CurrentKey->Results.reserve(RoundTitles.size());
        Statistics[KeyName] = CurrentKey;
    }
    return TestCase(CurrentKey);
}

StatisticsCollector::~StatisticsCollector()
{
    for(Statistics_t::iterator i = Statistics.begin(); i != Statistics.end(); i++)
        delete i->second;
}

void StatisticsCollector::ReserveRounds(size_t index)
{
    size_t i = RoundTitles.size();
    if (i > index) return;
    char buf[16];
    RoundTitles.resize(index+1);
    for(; i <= index; i++) {
        snprintf( buf, 15, "%u", unsigned(i+1) );
        RoundTitles[i] = buf;
    }
    for(Statistics_t::iterator i = Statistics.begin(); i != Statistics.end(); i++) {
        if(!i->second) printf("!!!'%s' = NULL\n", i->first.c_str());
        else i->second->Results.reserve(index+1);
    }
}

void StatisticsCollector::AddRoundResult(const TestCase &key, value_t v)
{
    ReserveRounds(key.access->Results.size());
    key.access->Results.push_back(v);
}

void StatisticsCollector::SetRoundTitle(size_t index, const char *fmt, ...)
{
    vargf2buff(buff, 128, fmt);
    ReserveRounds(index);
    RoundTitles[index] = buff;
}

void StatisticsCollector::AddStatisticValue(const TestCase &key, const char *type, const char *fmt, ...)
{
    vargf2buff(buff, 128, fmt);
    AnalysisTitles.insert(type);
    key.access->Analysis[type] = buff;
}

void StatisticsCollector::AddStatisticValue(const char *type, const char *fmt, ...)
{
    vargf2buff(buff, 128, fmt);
    AnalysisTitles.insert(type);
    CurrentKey->Analysis[type] = buff;
}

void StatisticsCollector::SetStatisticFormula(const char *name, const char *formula)
{
    Formulas[name] = formula;
}

void StatisticsCollector::SetTitle(const char *fmt, ...)
{
    vargf2buff(buff, 256, fmt);
    Title = buff;
}

string ExcelFormula(const string &fmt, size_t place, size_t rounds, bool is_horizontal)
{
    char buff[16];
    if(is_horizontal)
        snprintf(buff, 15, "RC[%u]:RC[%u]", unsigned(place), unsigned(place+rounds-1));
    else
        snprintf(buff, 15, "R[%u]C:R[%u]C", unsigned(place+1), unsigned(place+rounds));
    string result(fmt); size_t pos = 0;
    while ( (pos = result.find("ROUNDS", pos, 6)) != string::npos )
        result.replace(pos, 6, buff);
    return result;
}

void StatisticsCollector::Print(int dataOutput, const char *ModeName)
{
    FILE *OutputFile;
    if (dataOutput & StatisticsCollector::Stdout)
    {
        printf("\n-=# %s #=-\n", Title.c_str());
        if(SortMode == ByThreads)
            printf("    Name    |  #  | %s ", ModeName);
        else
            printf("    Name    | %s |  #  ", ModeName);
        for (AnalysisTitles_t::iterator i = AnalysisTitles.begin(); i != AnalysisTitles.end(); i++)
            printf("|%s", i->c_str()+1);

        for (Statistics_t::iterator i = Statistics.begin(); i != Statistics.end(); i++)
        {
            if(SortMode == ByThreads)
                printf("\n%12s|% 5d|%6s", i->second->Name.c_str(), i->second->Threads, i->second->Mode.c_str());
            else
                printf("\n%12s|%6s|% 5d", i->second->Name.c_str(), i->second->Mode.c_str(), i->second->Threads);
            Analysis_t &analisis = i->second->Analysis;
            AnalysisTitles_t::iterator t = AnalysisTitles.begin();
            for (Analysis_t::iterator a = analisis.begin(); a != analisis.end(); t++)
            {
                char fmt[8]; snprintf(fmt, 7, "|%% %us", unsigned(max(size_t(3), t->size())));
                if(*t != a->first)
                    printf(fmt, "");
                else {
                    printf(fmt, a->second.c_str()); a++;
                }
            }
        }
        printf("\n");
    }
    if (dataOutput & StatisticsCollector::HTMLFile)
    {
        if ((OutputFile = fopen((Name+".html").c_str(), "w+t")) != NULL)
        {
            char TimerBuff[100], DateBuff[100];
            GetTime(TimerBuff,sizeof(TimerBuff));
            GetDate(DateBuff,sizeof(DateBuff));
            fprintf(OutputFile, "<html><head>\n<title>%s</title>\n</head><body>\n", Title.c_str());
            //-----------------------
            fprintf(OutputFile, "<table id=\"h\" style=\"position:absolute;top:20\" border=1 cellspacing=0 cellpadding=2>\n");
            fprintf(OutputFile, "<tr><td><a name=hr href=#vr onclick=\"v.style.visibility='visible';"
                                "h.style.visibility='hidden';\">Flip[H]</a></td>"
                                "<td>%s</td><td>%s</td><td colspan=%u>%s</td>",
                DateBuff, TimerBuff, unsigned(AnalysisTitles.size() + RoundTitles.size()), Title.c_str());
            fprintf(OutputFile, "</tr>\n<tr bgcolor=#CCFFFF><td>Name</td><td>Threads</td><td>%s</td>", ModeName);
            for (AnalysisTitles_t::iterator i = AnalysisTitles.begin(); i != AnalysisTitles.end(); i++)
                fprintf(OutputFile, "<td>%s</td>", i->c_str()+1);
            for (size_t i = 0; i < RoundTitles.size(); i++)
                fprintf(OutputFile, "<td>%s</td>", RoundTitles[i].c_str());
            for (Statistics_t::iterator i = Statistics.begin(); i != Statistics.end(); i++)
            {
                fprintf(OutputFile, "</tr>\n<tr><td bgcolor=#CCFFCC>%s</td><td bgcolor=#CCFFCC>%d</td><td bgcolor=#CCFFCC>%4s</td>",
                    i->second->Name.c_str(), i->second->Threads, i->second->Mode.c_str());
                //statistics
                AnalysisTitles_t::iterator t = AnalysisTitles.begin();
                for (Analysis_t::iterator j = i->second->Analysis.begin(); j != i->second->Analysis.end(); t++)
                {
                    fprintf(OutputFile, "<td bgcolor=#FFFF99>%s</td>", (*t != j->first)?" ":(i->second->Analysis[j->first]).c_str());
                    if(*t == j->first) j++;
                }
                //data
                Results_t &r = i->second->Results;
                for (size_t k = 0; k < r.size(); k++)
                {
                    fprintf(OutputFile, "<td>");
                    fprintf(OutputFile, ResultsFmt, r[k]);
                    fprintf(OutputFile, "</td>");
                }
            }
            fprintf(OutputFile, "</tr>\n</table>\n");
            //////////////////////////////////////////////////////
            fprintf(OutputFile, "<table id=\"v\" style=\"visibility:hidden;position:absolute;top:20\" border=1 cellspacing=0 cellpadding=2>\n");
            fprintf(OutputFile, "<tr><td><a name=vr href=#hr onclick=\"h.style.visibility='visible';"
                                "v.style.visibility='hidden';\">Flip[V]</a></td>\n"
                                "<td>%s</td><td>%s</td><td colspan=%u>%s</td>", 
                DateBuff, TimerBuff, unsigned(max(Statistics.size()-2,size_t(1))), Title.c_str());

            fprintf(OutputFile, "</tr>\n<tr bgcolor=#CCFFCC><td bgcolor=#CCFFFF>Name</td>");
            for (Statistics_t::iterator i = Statistics.begin(); i != Statistics.end(); i++)
                fprintf(OutputFile, "<td>%s</td>", i->second->Name.c_str());
            fprintf(OutputFile, "</tr>\n<tr bgcolor=#CCFFCC><td bgcolor=#CCFFFF>Threads</td>");
            for (Statistics_t::iterator n = Statistics.begin(); n != Statistics.end(); n++)
                fprintf(OutputFile, "<td>%d</td>", n->second->Threads);
            fprintf(OutputFile, "</tr>\n<tr bgcolor=#CCFFCC><td bgcolor=#CCFFFF>%s</td>", ModeName);
            for (Statistics_t::iterator m = Statistics.begin(); m != Statistics.end(); m++)
                fprintf(OutputFile, "<td>%s</td>", m->second->Mode.c_str());

            for (AnalysisTitles_t::iterator t = AnalysisTitles.begin(); t != AnalysisTitles.end(); t++)
            {
                fprintf(OutputFile, "</tr>\n<tr bgcolor=#FFFF99><td bgcolor=#CCFFFF>%s</td>", t->c_str()+1);
                for (Statistics_t::iterator i = Statistics.begin(); i != Statistics.end(); i++)
                    fprintf(OutputFile, "<td>%s</td>", i->second->Analysis.count(*t)?i->second->Analysis[*t].c_str():" ");
            }

            for (size_t r = 0; r < RoundTitles.size(); r++)
            {
                fprintf(OutputFile, "</tr>\n<tr><td bgcolor=#CCFFFF>%s</td>", RoundTitles[r].c_str());
                for (Statistics_t::iterator i = Statistics.begin(); i != Statistics.end(); i++)
                {
                    Results_t &result = i->second->Results;
                    fprintf(OutputFile, "<td>");
                    if(result.size() > r)
                        fprintf(OutputFile, ResultsFmt, result[r]);
                    fprintf(OutputFile, "</td>");
                }
            }
            fprintf(OutputFile, "</tr>\n</table>\n</body></html>\n");
            fclose(OutputFile);
        }
    }
    if (dataOutput & StatisticsCollector::ExcelXML)
    {
        if ((OutputFile = fopen((Name+".xml").c_str(), "w+t")) == NULL) {
            printf("Can't open .xml file\n");
        } else {
            //vector<value_t> *TmpVect;
            //Statistics_t::iterator ii, i = Statistics.begin();
            //Analysis_t::iterator jj, j = i->second.Analysis.begin();
            char UserName[100];
            char SheetName[20];
            char TimerBuff[100], DateBuff[100];
#if _WIN32 || _WIN64
            strcpy(UserName,getenv("USERNAME"));
#else
            strcpy(UserName,getenv("USER"));
#endif
            //--------------------------------
            strcpy(SheetName,"Horizontal");
            GetTime(TimerBuff,sizeof(TimerBuff));
            GetDate(DateBuff,sizeof(DateBuff));
            //--------------------------
            fprintf(OutputFile, XMLHead, UserName, TimerBuff);
            fprintf(OutputFile, XMLStyles);
            fprintf(OutputFile, XMLBeginSheet, SheetName);
            fprintf(OutputFile, XMLNames,1,1,1,int(AnalysisTitles.size()+Formulas.size()+COUNT_PARAMETERS));
            fprintf(OutputFile, XMLBeginTable, int(RoundTitles.size()+Formulas.size()+AnalysisTitles.size()+COUNT_PARAMETERS+1/*title*/), int(Statistics.size()+1));
            fprintf(OutputFile, XMLBRow);
            fprintf(OutputFile, XMLCellTopName);
            fprintf(OutputFile, XMLCellTopThread);
            fprintf(OutputFile, XMLCellTopMode, ModeName);
            for (AnalysisTitles_t::iterator j = AnalysisTitles.begin(); j != AnalysisTitles.end(); j++)
                fprintf(OutputFile, XMLAnalysisTitle, j->c_str()+1);
            for (Formulas_t::iterator j = Formulas.begin(); j != Formulas.end(); j++)
                fprintf(OutputFile, XMLAnalysisTitle, j->first.c_str()+1);
            for (RoundTitles_t::iterator j = RoundTitles.begin(); j != RoundTitles.end(); j++)
                fprintf(OutputFile, XMLAnalysisTitle, j->c_str());
            fprintf(OutputFile, XMLCellEmptyWhite, Title.c_str());
            fprintf(OutputFile, XMLERow);
            //------------------------
            for (Statistics_t::iterator i = Statistics.begin(); i != Statistics.end(); i++)
            {
                fprintf(OutputFile, XMLBRow);
                fprintf(OutputFile, XMLCellName,  i->second->Name.c_str());
                fprintf(OutputFile, XMLCellThread,i->second->Threads);
                fprintf(OutputFile, XMLCellMode,  i->second->Mode.c_str());
                //statistics
                AnalysisTitles_t::iterator at = AnalysisTitles.begin();
                for (Analysis_t::iterator j = i->second->Analysis.begin(); j != i->second->Analysis.end(); at++)
                {
                    fprintf(OutputFile, XMLCellAnalysis, (*at != j->first)?"":(i->second->Analysis[j->first]).c_str());
                    if(*at == j->first) j++;
                }
                //formulas
                size_t place = 0;
                Results_t &v = i->second->Results;
                for (Formulas_t::iterator f = Formulas.begin(); f != Formulas.end(); f++, place++)
                    fprintf(OutputFile, XMLCellFormula, ExcelFormula(f->second, Formulas.size()-place, v.size(), true).c_str());
                //data
                for (size_t k = 0; k < v.size(); k++)
                {
                    fprintf(OutputFile, XMLCellData, v[k]);
                }
                if(v.size() < RoundTitles.size())
                    fprintf(OutputFile, XMLMergeRow, int(RoundTitles.size() - v.size()));
                fprintf(OutputFile, XMLERow);
            }
            //------------------------
            fprintf(OutputFile, XMLEndTable);
            fprintf(OutputFile, XMLWorkSheetProperties,1,1,3,3,int(RoundTitles.size()+AnalysisTitles.size()+Formulas.size()+COUNT_PARAMETERS));
            fprintf(OutputFile, XMLAutoFilter,1,1,1,int(AnalysisTitles.size()+Formulas.size()+COUNT_PARAMETERS));
            fprintf(OutputFile, XMLEndWorkSheet);
            //----------------------------------------
            strcpy(SheetName,"Vertical");
            fprintf(OutputFile, XMLBeginSheet, SheetName);
            fprintf(OutputFile, XMLNames, int(Formulas.size()+AnalysisTitles.size()+COUNT_PARAMETERS+2),2,int(AnalysisTitles.size()+Formulas.size()+COUNT_PARAMETERS+2),int(Statistics.size()+1));
            fprintf(OutputFile, XMLBeginTable, int(max(Statistics.size()+1, size_t(7))), int(RoundTitles.size()+AnalysisTitles.size()+Formulas.size()+COUNT_PARAMETERS+2));
            //fprintf(OutputFile, XMLColumsVerticalTable, Statistics.size()+1);
            //----------------------------------------

            fprintf(OutputFile, XMLBRow);
            fprintf(OutputFile, XMLNameAndTime, Name.c_str(), TimerBuff, DateBuff);
            fprintf(OutputFile, XMLTableParamAndTitle, int(Statistics.size()), int(AnalysisTitles.size()), int(RoundTitles.size()), Title.c_str());
            fprintf(OutputFile, XMLERow);
            fprintf(OutputFile, XMLBRow);
            //-------------------
            fprintf(OutputFile, XMLCellTopName);
            for (Statistics_t::iterator i = Statistics.begin(); i != Statistics.end(); i++)
                fprintf(OutputFile, XMLCellName, i->second->Name.c_str());
            fprintf(OutputFile, XMLERow);
            fprintf(OutputFile, XMLBRow);
            fprintf(OutputFile, XMLCellTopThread);
            for (Statistics_t::iterator i = Statistics.begin(); i != Statistics.end(); i++)
                fprintf(OutputFile, XMLCellThread, i->second->Threads);
            fprintf(OutputFile, XMLERow);
            fprintf(OutputFile, XMLBRow);
            fprintf(OutputFile, XMLCellTopMode, ModeName);
            for (Statistics_t::iterator i = Statistics.begin(); i != Statistics.end(); i++)
                fprintf(OutputFile, XMLCellMode, i->second->Mode.c_str());
            fprintf(OutputFile, XMLERow);
            //-----------------
            for (AnalysisTitles_t::iterator t = AnalysisTitles.begin(); t != AnalysisTitles.end(); t++)
            {
                fprintf(OutputFile, XMLBRow);
                fprintf(OutputFile, XMLAnalysisTitle, t->c_str()+1);
                for (Statistics_t::iterator i = Statistics.begin(); i != Statistics.end(); i++)
                    fprintf(OutputFile, XMLCellAnalysis, i->second->Analysis.count(*t)?(i->second->Analysis[*t]).c_str():"");
                fprintf(OutputFile, XMLERow);
            }
            //-------------------------------------
            for (Formulas_t::iterator t = Formulas.begin(); t != Formulas.end(); t++)
            {
                fprintf(OutputFile, XMLBRow);
                fprintf(OutputFile, XMLAnalysisTitle, t->first.c_str()+1);
                size_t place = 0;
                for (Statistics_t::iterator i = Statistics.begin(); i != Statistics.end(); i++)
                    fprintf(OutputFile, XMLCellAnalysis, ExcelFormula(t->second, Formulas.size()-place, i->second->Results.size(), false).c_str());
                fprintf(OutputFile, XMLERow);
            }
            //--------------------------------------
            fprintf(OutputFile, XMLBRow);
            fprintf(OutputFile, XMLCellEmptyWhite, "Result");
            fprintf(OutputFile, XMLERow);
            
            for (size_t k = 0; k < RoundTitles.size(); k++)
            {
                fprintf(OutputFile, XMLBRow);
                fprintf(OutputFile, XMLAnalysisTitle, RoundTitles[k].c_str());
                for (Statistics_t::iterator i = Statistics.begin(); i != Statistics.end(); i++)
                    if(i->second->Results.size() > k)
                        fprintf(OutputFile, XMLCellData, i->second->Results[k]);
                    else
                        fprintf(OutputFile, XMLCellEmptyWhite, "");
                fprintf(OutputFile, XMLERow);
            }
            fprintf(OutputFile, XMLEndTable);
            //----------------------------------------
            fprintf(OutputFile, XMLWorkSheetProperties, int(Formulas.size()+AnalysisTitles.size()+COUNT_PARAMETERS+2), int(Formulas.size()+AnalysisTitles.size()+COUNT_PARAMETERS+2),1,1,6);
            fprintf(OutputFile, XMLAutoFilter, int(Formulas.size()+AnalysisTitles.size()+COUNT_PARAMETERS+2),2, int(Formulas.size()+AnalysisTitles.size()+COUNT_PARAMETERS+2), int(Statistics.size()+1));
            //----------------------------------------
            fprintf(OutputFile, XMLEndWorkSheet);
            fprintf(OutputFile, XMLEndWorkbook);
            fclose(OutputFile);
        }
    }
}

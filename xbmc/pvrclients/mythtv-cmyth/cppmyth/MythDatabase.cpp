#include "MythDatabase.h"
#include "MythChannel.h"
#include "MythTimer.h"
#include "client.h"

using namespace ADDON;

/*
*								MythDatabase
*/

MythDatabase::MythDatabase()
  :m_database_t()
{
}


MythDatabase::MythDatabase(CStdString server,CStdString database,CStdString user,CStdString password):
m_database_t(new MythPointerThreadSafe<cmyth_database_t>())
{
  char *cserver=strdup(server.c_str());
  char *cdatabase=strdup(database.c_str());
  char *cuser=strdup(user.c_str());
  char *cpassword=strdup(password.c_str());

  *m_database_t=(CMYTH->DatabaseInit(cserver,cdatabase,cuser,cpassword));

  free(cserver);
  free(cdatabase);
  free(cuser);
  free(cpassword);
}

bool  MythDatabase::TestConnection(CStdString &msg)
{
  char* cmyth_msg;
  m_database_t->Lock();
  bool retval=CMYTH->MysqlTestdbConnection(*m_database_t,&cmyth_msg)==1;
  msg=cmyth_msg;
  free(cmyth_msg);
  m_database_t->Unlock();
  return retval;
}


std::map<int,MythChannel> MythDatabase::ChannelList()
{
  std::map<int,MythChannel> retval;
  m_database_t->Lock();
  cmyth_chanlist_t cChannels=CMYTH->MysqlGetChanlist(*m_database_t);
  m_database_t->Unlock();
  int nChannels=CMYTH->ChanlistGetCount(cChannels);
  for(int i=0;i<nChannels;i++)
  {
    cmyth_channel_t chan=CMYTH->ChanlistGetItem(cChannels,i);
    int chanid=CMYTH->ChannelChanid(chan);
    retval.insert(std::pair<int,MythChannel>(chanid,MythChannel(chan,1==CMYTH->MysqlIsRadio(*m_database_t,chanid))));
  }
  CMYTH->RefRelease(cChannels);
  return retval;
}

std::vector<MythProgram> MythDatabase::GetGuide(time_t starttime, time_t endtime)
{
  MythProgram *programs=0;
  m_database_t->Lock();
  int len=CMYTH->MysqlGetGuide(*m_database_t,&programs,starttime,endtime);
  m_database_t->Unlock();
  if(len==0)
    return std::vector<MythProgram>();
  std::vector<MythProgram> retval(programs,programs+len);
  CMYTH->RefRelease(programs);
  return retval;
}

std::vector<MythTimer> MythDatabase::GetTimers()
{
  std::vector<MythTimer> retval;
  m_database_t->Lock();
  cmyth_timerlist_t timers=CMYTH->MysqlGetTimers(*m_database_t);
  m_database_t->Unlock();
  int nTimers=CMYTH->TimerlistGetCount(timers);
  for(int i=0;i<nTimers;i++)
  {
    cmyth_timer_t timer=CMYTH->TimerlistGetItem(timers,i);
    retval.push_back(MythTimer(timer));
  }
  CMYTH->RefRelease(timers);
  return retval;
}

int MythDatabase::AddTimer(MythTimer &timer)
{
  m_database_t->Lock();
  int retval=CMYTH->MysqlAddTimer(*m_database_t,timer.ChanID(),timer.m_channame.Buffer(),timer.m_description.Buffer(),timer.StartTime(), timer.EndTime(),timer.m_title.Buffer(),timer.m_category.Buffer(),timer.Type(),timer.m_subtitle.Buffer(),timer.Priority(),timer.StartOffset(),timer.EndOffset(),timer.SearchType(),timer.Inactive()?1:0);
  timer.m_recordid=retval;
  m_database_t->Unlock();
  return retval;
}

  bool MythDatabase::DeleteTimer(int recordid)
  {
  m_database_t->Lock();
  bool retval= CMYTH->MysqlDeleteTimer(*m_database_t,recordid)==0;
  m_database_t->Unlock();
  return retval;
  }

  bool MythDatabase::UpdateTimer(MythTimer &timer)
  {
  m_database_t->Lock();
  bool retval = CMYTH->MysqlUpdateTimer(*m_database_t,timer.RecordID(),timer.ChanID(),timer.m_channame.Buffer(),timer.m_description.Buffer(),timer.StartTime(), timer.EndTime(),timer.m_title.Buffer(),timer.m_category.Buffer(),timer.Type(),timer.m_subtitle.Buffer(),timer.Priority(),timer.StartOffset(),timer.EndOffset(),timer.SearchType(),timer.Inactive()?1:0)==0;
  m_database_t->Unlock();
  return retval;
  }

  bool MythDatabase::FindProgram(const time_t starttime,const int channelid,CStdString &title,MythProgram* pprogram)
  {
    m_database_t->Lock();
    bool retval=CMYTH->MysqlGetProgFinderTimeTitleChan(*m_database_t,pprogram,title.Buffer(),starttime,channelid)>0;
    m_database_t->Unlock();
    return retval;
   }

  boost::unordered_map< CStdString, std::vector< int > > MythDatabase::GetChannelGroups()
  {
  boost::unordered_map< CStdString, std::vector< int > > retval;
    m_database_t->Lock();
  cmyth_channelgroups_t *cg =0;
  int len = CMYTH->MysqlGetChannelgroups(*m_database_t,&cg);
  if(!cg)
    return retval;
  for(int i=0;i<len;i++)
  {
    MythChannelGroup changroup;
    changroup.first=cg[i].channelgroup;
    int* chanid=0;
    int numchan=CMYTH->MysqlGetChannelidsInGroup(*m_database_t,cg[i].ID,&chanid);
    if(numchan)
    {
      changroup.second=std::vector<int>(chanid,chanid+numchan);
      CMYTH->RefRelease(chanid);
    }
    else 
      changroup.second=std::vector<int>();
    
    retval.insert(changroup);
  }
  CMYTH->RefRelease(cg);
  m_database_t->Unlock();
  return retval;
  }

  std::map< int, std::vector< int > > MythDatabase::SourceList()
  {
  std::map< int, std::vector< int > > retval;
  cmyth_rec_t *r=0;
  m_database_t->Lock();
  int len=CMYTH->MysqlGetRecorderList(*m_database_t,&r);
  for(int i=0;i<len;i++)
  {
    std::map< int, std::vector< int > >::iterator it=retval.find(r[i].sourceid);
    if(it!=retval.end())
      it->second.push_back(r[i].recid);
    else
      retval[r[i].sourceid]=std::vector<int>(1,r[i].recid);
    }
  CMYTH->RefRelease(r);
  r=0;
  m_database_t->Unlock();
  return retval;
  }

  bool MythDatabase::IsNull()
  {
    if(m_database_t==NULL)
      return true;
    return *m_database_t==NULL;
  }

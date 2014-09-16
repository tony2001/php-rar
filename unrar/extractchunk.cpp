#include "rar.hpp"

bool CmdExtract::ExtractCurrentFileChunkInit(CommandData *Cmd,
                                             Archive &Arc,
                                             size_t HeaderSize,
                                             bool &Repeat)
{
  char Command = 'T';

  Cmd->DllError=0;
  Repeat = false;

  //turn on checks reserved for the first files extracted from an archive?
  FirstFile = true;

  if (HeaderSize==0) {
    if (DataIO.UnpVolume)
    {
#ifdef NOVOLUME
      return(false);
#else
      if (!MergeArchive(Arc,&DataIO,false,Command)) //command irrelevant
      {
        ErrHandler.SetErrorCode(RARX_WARNING);
        return false;
      }
#endif
    }
    else
      return false;
  }

  int HeadType=Arc.GetHeaderType();
  if (HeadType!=HEAD_FILE)
    return false;

  DataIO.SetUnpackToMemory((byte*) this->Buffer, this->BufferSize);
  DataIO.SetSkipUnpCRC(true);
  DataIO.SetCurrentCommand(Command);
  //will still write to mem, as we've told it, but if I've screwed up the
  //there'll be no operations in the filesystem
  DataIO.SetTestMode(true);

  if ((Arc.FileHead.Flags & (LHD_SPLIT_BEFORE/*|LHD_SOLID*/)) && FirstFile)
  {
    wchar CurVolName[NM];
    wcsncpyz(CurVolName,ArcName,ASIZE(CurVolName));
    VolNameToFirstName(ArcName,ArcName,ASIZE(ArcName),Arc.NewNumbering);

    if (wcsicomp(ArcName,CurVolName)!=0 && FileExist(ArcName))
    {
      *ArcName=0;
      Repeat=true;
      ErrHandler.SetErrorCode(RARX_WARNING);
      /* Actually known. The problem is that the file doesn't start on this volume. */
      Cmd->DllError = ERAR_UNKNOWN;
      return false;
    }
  }
  DataIO.UnpVolume=(Arc.FileHead.Flags & LHD_SPLIT_AFTER)!=0;
  DataIO.NextVolumeMissing=false;

  Arc.Seek(Arc.NextBlockPos - Arc.FileHead.PackSize, SEEK_SET);

  if ((Arc.FileHead.Flags & LHD_PASSWORD)!=0)
  {
    if (!Cmd->Password.IsSet())
    {
      if (Cmd->Callback!=NULL)
      {
        wchar PasswordW[MAXPASSWORD];
        *PasswordW=0;
        if (Cmd->Callback(UCM_NEEDPASSWORDW,Cmd->UserData,(LPARAM)PasswordW,ASIZE(PasswordW))==-1)
          *PasswordW=0;
        if (*PasswordW==0)
        {
          char PasswordA[MAXPASSWORD];
          *PasswordA=0;
          if (Cmd->Callback(UCM_NEEDPASSWORD,Cmd->UserData,(LPARAM)PasswordA,ASIZE(PasswordA))==-1)
            *PasswordA=0;
          GetWideName(PasswordA,NULL,PasswordW,ASIZE(PasswordW));
          cleandata(PasswordA,sizeof(PasswordA));
        }
        Cmd->Password.Set(PasswordW);
        cleandata(PasswordW,sizeof(PasswordW));
      }
      if (!Cmd->Password.IsSet())
      {
        Cmd->DllError = ERAR_MISSING_PASSWORD; //added by me
        return false;
      }
    }
    Password=Cmd->Password;
  }

  if (Arc.FileHead.UnpVer<13 || Arc.FileHead.UnpVer>VER_UNPACK)
  {
    ErrHandler.SetErrorCode(RARX_WARNING);
    Cmd->DllError=ERAR_UNKNOWN_FORMAT;
    return false;
  }

  if (IsLink(Arc.FileHead.FileAttr))
    return true;
  
  if (Arc.IsArcDir())
      return true;

  DataIO.CurUnpRead=0;
  DataIO.CurUnpWrite=0;

  byte PswCheck[SIZE_PSWCHECK];
  DataIO.SetEncryption(false,Arc.FileHead.CryptMethod,&Password,
		  Arc.FileHead.SaltSet ? Arc.FileHead.Salt:NULL,
		  Arc.FileHead.InitV,Arc.FileHead.Lg2Count,
		  PswCheck,Arc.FileHead.HashKey);
  if (Arc.FileHead.Encrypted && Arc.FileHead.UsePswCheck &&
      memcmp(Arc.FileHead.PswCheck,PswCheck,SIZE_PSWCHECK)!=0 &&
      !Arc.BrokenHeader)
  {
    uiMsg(UIERROR_BADPSW,Arc.FileName);
    ErrHandler.SetErrorCode(RARX_BADPWD);
	return false;
  }
/*
  DataIO.SetEncryption(
    (Arc.FileHead.Flags & LHD_PASSWORD) ? Arc.FileHead.UnpVer : 0, &Password,
    (Arc.FileHead.Flags & LHD_SALT) ? Arc.FileHead.Salt : NULL, false,
    Arc.FileHead.UnpVer >= 36);
*/
  DataIO.SetPackedSizeToRead(Arc.FileHead.PackSize);
  DataIO.SetSkipUnpCRC(true);
  DataIO.SetFiles(&Arc, NULL);

  return true;
}

bool CmdExtract::ExtractCurrentFileChunk(CommandData *Cmd, Archive &Arc,
                                         size_t *ReadSize,
                                         int *finished)
{
  if (IsLink(Arc.FileHead.FileAttr) || Arc.IsArcDir()) {
    *ReadSize = 0;
    *finished = TRUE;
    return true;
  }

  DataIO.SetUnpackToMemory((byte*) this->Buffer, this->BufferSize);

  if (Arc.FileHead.Method==0) {
    UnstoreFile(DataIO, this->BufferSize);
    /* not very sophisticated and may result in a subsequent
     * unnecessary call to this function (and probably will if
     * the buffer size is chosen so that it just fits for small
     * files) */
    *finished = (DataIO.GetUnpackToMemorySizeLeft() > 0);
  }
  else
  {
    Unp->Init(Arc.FileHead.WinSize,Arc.FileHead.Solid);
    Unp->SetDestSize(Arc.FileHead.UnpSize);
    if (Arc.FileHead.UnpVer<=15)
      Unp->DoUnpack(15,FileCount>1 && Arc.Solid, this->Buffer != NULL);
    else
      Unp->DoUnpack(Arc.FileHead.UnpVer,Arc.FileHead.Solid, this->Buffer != NULL);
    *finished = Unp->IsFileExtracted();
  }
  *ReadSize = this->BufferSize - DataIO.GetUnpackToMemorySizeLeft();

  return true;
}

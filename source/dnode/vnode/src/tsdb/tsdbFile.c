/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "tsdb.h"

int32_t tPutHeadFile(uint8_t *p, SHeadFile *pHeadFile) {
  int32_t n = 0;

  n += tPutI64v(p ? p + n : p, pHeadFile->commitID);
  n += tPutI64v(p ? p + n : p, pHeadFile->size);
  n += tPutI64v(p ? p + n : p, pHeadFile->offset);

  return n;
}

static int32_t tGetHeadFile(uint8_t *p, SHeadFile *pHeadFile) {
  int32_t n = 0;

  n += tGetI64v(p + n, &pHeadFile->commitID);
  n += tGetI64v(p + n, &pHeadFile->size);
  n += tGetI64v(p + n, &pHeadFile->offset);

  return n;
}

int32_t tPutDataFile(uint8_t *p, SDataFile *pDataFile) {
  int32_t n = 0;

  n += tPutI64v(p ? p + n : p, pDataFile->commitID);
  n += tPutI64v(p ? p + n : p, pDataFile->size);

  return n;
}

static int32_t tGetDataFile(uint8_t *p, SDataFile *pDataFile) {
  int32_t n = 0;

  n += tGetI64v(p + n, &pDataFile->commitID);
  n += tGetI64v(p + n, &pDataFile->size);

  return n;
}

int32_t tPutSstFile(uint8_t *p, SSstFile *pSstFile) {
  int32_t n = 0;

  n += tPutI64v(p ? p + n : p, pSstFile->commitID);
  n += tPutI64v(p ? p + n : p, pSstFile->size);
  n += tPutI64v(p ? p + n : p, pSstFile->offset);

  return n;
}

static int32_t tGetSstFile(uint8_t *p, SSstFile *pSstFile) {
  int32_t n = 0;

  n += tGetI64v(p + n, &pSstFile->commitID);
  n += tGetI64v(p + n, &pSstFile->size);
  n += tGetI64v(p + n, &pSstFile->offset);

  return n;
}

int32_t tPutSmaFile(uint8_t *p, SSmaFile *pSmaFile) {
  int32_t n = 0;

  n += tPutI64v(p ? p + n : p, pSmaFile->commitID);
  n += tPutI64v(p ? p + n : p, pSmaFile->size);

  return n;
}

static int32_t tGetSmaFile(uint8_t *p, SSmaFile *pSmaFile) {
  int32_t n = 0;

  n += tGetI64v(p + n, &pSmaFile->commitID);
  n += tGetI64v(p + n, &pSmaFile->size);

  return n;
}

// EXPOSED APIS ==================================================
void tsdbHeadFileName(STsdb *pTsdb, SDiskID did, int32_t fid, SHeadFile *pHeadF, char fname[]) {
  snprintf(fname, TSDB_FILENAME_LEN - 1, "%s%s%s%sv%df%dver%" PRId64 "%s", tfsGetDiskPath(pTsdb->pVnode->pTfs, did),
           TD_DIRSEP, pTsdb->path, TD_DIRSEP, TD_VID(pTsdb->pVnode), fid, pHeadF->commitID, ".head");
}

void tsdbDataFileName(STsdb *pTsdb, SDiskID did, int32_t fid, SDataFile *pDataF, char fname[]) {
  snprintf(fname, TSDB_FILENAME_LEN - 1, "%s%s%s%sv%df%dver%" PRId64 "%s", tfsGetDiskPath(pTsdb->pVnode->pTfs, did),
           TD_DIRSEP, pTsdb->path, TD_DIRSEP, TD_VID(pTsdb->pVnode), fid, pDataF->commitID, ".data");
}

void tsdbSstFileName(STsdb *pTsdb, SDiskID did, int32_t fid, SSstFile *pSstF, char fname[]) {
  snprintf(fname, TSDB_FILENAME_LEN - 1, "%s%s%s%sv%df%dver%" PRId64 "%s", tfsGetDiskPath(pTsdb->pVnode->pTfs, did),
           TD_DIRSEP, pTsdb->path, TD_DIRSEP, TD_VID(pTsdb->pVnode), fid, pSstF->commitID, ".sst");
}

void tsdbSmaFileName(STsdb *pTsdb, SDiskID did, int32_t fid, SSmaFile *pSmaF, char fname[]) {
  snprintf(fname, TSDB_FILENAME_LEN - 1, "%s%s%s%sv%df%dver%" PRId64 "%s", tfsGetDiskPath(pTsdb->pVnode->pTfs, did),
           TD_DIRSEP, pTsdb->path, TD_DIRSEP, TD_VID(pTsdb->pVnode), fid, pSmaF->commitID, ".sma");
}

bool tsdbDelFileIsSame(SDelFile *pDelFile1, SDelFile *pDelFile2) { return pDelFile1->commitID == pDelFile2->commitID; }

int32_t tsdbDFileRollback(STsdb *pTsdb, SDFileSet *pSet, EDataFileT ftype) {
  int32_t   code = 0;
  int64_t   size;
  int64_t   n;
  TdFilePtr pFD;
  char      fname[TSDB_FILENAME_LEN];
  char      hdr[TSDB_FHDR_SIZE] = {0};

  // truncate
  switch (ftype) {
    case TSDB_DATA_FILE:
      size = pSet->pDataF->size;
      tsdbDataFileName(pTsdb, pSet->diskId, pSet->fid, pSet->pDataF, fname);
      tPutDataFile(hdr, pSet->pDataF);
      break;
    case TSDB_SMA_FILE:
      size = pSet->pSmaF->size;
      tsdbSmaFileName(pTsdb, pSet->diskId, pSet->fid, pSet->pSmaF, fname);
      tPutSmaFile(hdr, pSet->pSmaF);
      break;
    default:
      ASSERT(0);
  }

  taosCalcChecksumAppend(0, hdr, TSDB_FHDR_SIZE);

  // open
  pFD = taosOpenFile(fname, TD_FILE_WRITE);
  if (pFD == NULL) {
    code = TAOS_SYSTEM_ERROR(errno);
    goto _err;
  }

  // ftruncate
  if (taosFtruncateFile(pFD, LOGIC_TO_FILE_SIZE(size, TSDB_DEFAULT_PAGE_SIZE)) < 0) {
    code = TAOS_SYSTEM_ERROR(errno);
    goto _err;
  }

  // update header
  n = taosLSeekFile(pFD, 0, SEEK_SET);
  if (n < 0) {
    code = TAOS_SYSTEM_ERROR(errno);
    goto _err;
  }

  n = taosWriteFile(pFD, hdr, TSDB_FHDR_SIZE);
  if (n < 0) {
    code = TAOS_SYSTEM_ERROR(errno);
    goto _err;
  }

  // sync
  if (taosFsyncFile(pFD) < 0) {
    code = TAOS_SYSTEM_ERROR(errno);
    goto _err;
  }

  // close
  taosCloseFile(&pFD);

  return code;

_err:
  tsdbError("vgId:%d, tsdb rollback file failed since %s", TD_VID(pTsdb->pVnode), tstrerror(code));
  return code;
}

int32_t tPutDFileSet(uint8_t *p, SDFileSet *pSet) {
  int32_t n = 0;

  n += tPutI32v(p ? p + n : p, pSet->diskId.level);
  n += tPutI32v(p ? p + n : p, pSet->diskId.id);
  n += tPutI32v(p ? p + n : p, pSet->fid);

  // data
  n += tPutHeadFile(p ? p + n : p, pSet->pHeadF);
  n += tPutDataFile(p ? p + n : p, pSet->pDataF);
  n += tPutSmaFile(p ? p + n : p, pSet->pSmaF);

  // sst
  n += tPutU8(p ? p + n : p, pSet->nSstF);
  for (int32_t iSst = 0; iSst < pSet->nSstF; iSst++) {
    n += tPutSstFile(p ? p + n : p, pSet->aSstF[iSst]);
  }

  return n;
}

int32_t tGetDFileSet(uint8_t *p, SDFileSet *pSet) {
  int32_t n = 0;

  n += tGetI32v(p + n, &pSet->diskId.level);
  n += tGetI32v(p + n, &pSet->diskId.id);
  n += tGetI32v(p + n, &pSet->fid);

  // head
  pSet->pHeadF = (SHeadFile *)taosMemoryCalloc(1, sizeof(SHeadFile));
  if (pSet->pHeadF == NULL) {
    return -1;
  }
  pSet->pHeadF->nRef = 1;
  n += tGetHeadFile(p + n, pSet->pHeadF);

  // data
  pSet->pDataF = (SDataFile *)taosMemoryCalloc(1, sizeof(SDataFile));
  if (pSet->pDataF == NULL) {
    return -1;
  }
  pSet->pDataF->nRef = 1;
  n += tGetDataFile(p + n, pSet->pDataF);

  // sma
  pSet->pSmaF = (SSmaFile *)taosMemoryCalloc(1, sizeof(SSmaFile));
  if (pSet->pSmaF == NULL) {
    return -1;
  }
  pSet->pSmaF->nRef = 1;
  n += tGetSmaFile(p + n, pSet->pSmaF);

  // sst
  n += tGetU8(p + n, &pSet->nSstF);
  for (int32_t iSst = 0; iSst < pSet->nSstF; iSst++) {
    pSet->aSstF[iSst] = (SSstFile *)taosMemoryCalloc(1, sizeof(SSstFile));
    if (pSet->aSstF[iSst] == NULL) {
      return -1;
    }
    pSet->aSstF[iSst]->nRef = 1;
    n += tGetSstFile(p + n, pSet->aSstF[iSst]);
  }

  return n;
}

// SDelFile ===============================================
void tsdbDelFileName(STsdb *pTsdb, SDelFile *pFile, char fname[]) {
  snprintf(fname, TSDB_FILENAME_LEN - 1, "%s%s%s%sv%dver%" PRId64 "%s", tfsGetPrimaryPath(pTsdb->pVnode->pTfs),
           TD_DIRSEP, pTsdb->path, TD_DIRSEP, TD_VID(pTsdb->pVnode), pFile->commitID, ".del");
}

int32_t tPutDelFile(uint8_t *p, SDelFile *pDelFile) {
  int32_t n = 0;

  n += tPutI64v(p ? p + n : p, pDelFile->commitID);
  n += tPutI64v(p ? p + n : p, pDelFile->size);
  n += tPutI64v(p ? p + n : p, pDelFile->offset);

  return n;
}

int32_t tGetDelFile(uint8_t *p, SDelFile *pDelFile) {
  int32_t n = 0;

  n += tGetI64v(p + n, &pDelFile->commitID);
  n += tGetI64v(p + n, &pDelFile->size);
  n += tGetI64v(p + n, &pDelFile->offset);

  return n;
}

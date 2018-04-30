/*******************************************************************************
 |    bbio_regular.h
 |
 |    Copyright IBM Corporation 2016,2017. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/


#ifndef BB_BBIO_REGULAR_H_
#define BB_BBIO_REGULAR_H_

#include <map>

#include "bbio.h"
#include "fh.h"

/*******************************************************************************
 | Forward declarations
 *******************************************************************************/

class bbioop
{
  public:
    int    fd;
    bool   isWrite;
    off_t  offset;
    size_t len;
    char*  buffer;

    // handle detaching buffer for pwrites....
    const char* xferbuffer;

    // handle return for preads....
    ssize_t*   completion_rc;
    sem_t*     completion;
};

extern void* bbioRegFileManager(void* vptr);

class bbioRegularData : public filehandleData
{
  public:
    pthread_t managingThread;

    pthread_mutex_t       lock;
    sem_t                 numPendingOps;
    map<off_t, bbioop>    pendingops;

    bbioRegularData()
    {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
//        pthread_attr_setstacksize(&attr, 2*getpagesize());

        sem_init(&numPendingOps,0,0);
        pthread_mutex_init(&lock, NULL);
        pthread_create(&managingThread, &attr, bbioRegFileManager, this);
    }
    virtual ~bbioRegularData()
    {
        pthread_cancel(managingThread);
    }
    int addOp(bbioop& op);
};

class BBIO_Regular : public BBIO
{
  public:
    //  A BBIO_Regular() object is instantiated for each remote non-shared file
    //  in the list of files in the associated transfer definition.
    BBIO_Regular(int32_t pContribId, BBTransferDef* pTransferDef) :
          BBIO(pContribId, pTransferDef)
    {
        SINGLETHREADIO = config.get(resolveServerConfigKey("singleThreadIO"), false);
    };

    /**
     * \brief Destructor
     */
    virtual ~BBIO_Regular()
    {
        for(auto& fhpair : fhmap) { delete fhpair.second; }
    };

    // Virtual methods

    // Returns the filehandle for the input file index
    virtual filehandle* getFileHandle(const uint32_t pFileIndex);

    // Returns the number of writes performed for the file without an fsync
    virtual uint32_t getNumWritesNoSync(const uint32_t pFileIndex);

    // Returns the total size of data that has been writen to the file without an fsync
    virtual size_t getTotalSizeWritesNoSync(const uint32_t pFileIndex);

    // Increments the number of writes performed for the file without an fsync
    virtual void incrNumWritesNoSync(const uint32_t pFileIndex, const uint32_t pValue=1);

    // Increments the total size of data that has been writen to the file without an fsync
    virtual void incrTotalSizeWritesNoSync(const uint32_t pFileIndex, const size_t pValue);

    // Sets the number of writes performed for the file without an fsync
    virtual void setNumWritesNoSync(const uint32_t pFileIndex, const uint32_t pValue);

    // Sets the total size of data that has been writen to the file without an fsync
    virtual void setTotalSizeWritesNoSync(const uint32_t pFileIndex, const size_t pValue);

    // Virtual method to close a file in the parallel file system.
    virtual int close(uint32_t pFileIndex);

    // Virtual method to close all file handles (Clean-up for abnormal start transfer)
    virtual void closeAllFileHandles();

    // Virtual method to open a file in the parallel file system.
    virtual int open(uint32_t pFileIndex, uint64_t pBBFileFlags, const string& pFileName, const mode_t pMode=0);

    // Virtual method to fstat a file in the parallel file system.
    virtual int fstat(uint32_t pFileIndex, struct stat* pStats);

    // Virtual method to read from the parallel file system.  Return value is number of bytes read.
    virtual ssize_t pread(uint32_t pFileIndex, char* pBuffer, size_t pMaxBytesToRead, off_t& pOffset);

    // Virtual method to synchronize a file that was written to the parallel file system.
    virtual int fsync(uint32_t pFileIndex);

    // Virtual method to write to the parallel file system.  Return value is number of bytes written.
    virtual ssize_t pwrite(uint32_t pFileIndex, const char* pBuffer, size_t pMaxBytesToWrite, off_t& pOffset);

    virtual unsigned int getBacklog(uint32_t pFileIndex);

  private:
    map<uint32_t,filehandle*> fhmap;
    bool SINGLETHREADIO;
};

#endif /* BB_BBIO_REGULAR_H_ */

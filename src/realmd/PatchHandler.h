#ifndef _PATCHHANDLER_H_
#define _PATCHHANDLER_H_

#include <ace/Basic_Types.h>
#include <ace/Synch_Traits.h>
#include <ace/Svc_Handler.h>
#include <ace/SOCK_Stream.h>
#include <ace/Message_Block.h>
#include <ace/Auto_Ptr.h>
#include <map>

#include <openssl/bn.h>
#include <openssl/md5.h>

/**
 * @brief Caches MD5 hash of client patches present on the server
 */
class PatchCache
{
    public:
        ~PatchCache();
        PatchCache();

        static PatchCache* instance();

        struct PATCH_INFO
        {
            ACE_UINT8 md5[MD5_DIGEST_LENGTH];
        };

        typedef std::map<std::string, PATCH_INFO*> Patches;

        Patches::const_iterator begin() const
        {
            return patches_.begin();
        }

        Patches::const_iterator end() const
        {
            return patches_.end();
        }

        void LoadPatchMD5(const char*);
        bool GetHash(const char * pat, ACE_UINT8 mymd5[MD5_DIGEST_LENGTH]);

    private:
        void LoadPatchesInfo();
        Patches patches_;

};

class PatchHandler: public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>
{
    protected:
        typedef ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH> Base;

    public:
        PatchHandler(ACE_HANDLE socket, ACE_HANDLE patch);
        virtual ~PatchHandler();

        int open(void* = 0);

    protected:
        virtual int svc(void);

    private:
        ACE_HANDLE patch_fd_;

};

#endif /* _BK_PATCHHANDLER_H__ */


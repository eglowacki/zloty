///////////////////////////////////////////////////////////////////////
// MediaAssetImpl.h
//
//  Copyright 11/24/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//
//  #include "MediaAssetImpl.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef MEDIA_MEDIA_ASSET_IMPL_H
#define MEDIA_MEDIA_ASSET_IMPL_H

#ifndef MEDIA_ASSET_INCLUDE_IMPLEMENTATION
    #error "Do not include this file explicitly."
#endif // MEDIA_ASSET_INCLUDE_IMPLEMENTATION



namespace eg
{
    namespace internal
    {
        struct HeaderMarker
        {
            HeaderMarker(size_t& size, std::ostream& ostream) : Size(size), Ostream(ostream), mBegLen(0)
            {
                mBegLen = eg::GetOStreamSize(Ostream);
            }

            ~HeaderMarker()
            {
                size_t endLen = eg::GetOStreamSize(Ostream);
                Size = endLen - mBegLen;
            }

            size_t& Size;
            std::ostream& Ostream;
            size_t mBegLen;
        };

        /*!
        This class is used to get trigger on deletion of istream
        which need to be copied to mOStream.
        */
        class StreamDeleter
        {
        public:
            StreamDeleter(VirtualFileFactory::ostream_t ostream, size_t headerSize) :
                mOStream(ostream),
                mHeaderSize(headerSize)
            {
            }

            void operator()(void *p)
            {
                // if this is 0, then OnAdjustDataStream() method was never called
                // which should be called when Assset is created for the first time
                std::istream *pistream = (std::istream *)p;
                pistream->seekg(static_cast<long>(mHeaderSize), std::ios::cur);

                eg::CopyStream(*pistream, *mOStream);
                mOStream->flush();
                delete p;
            }

        private:
            VirtualFileFactory::ostream_t mOStream;
            size_t mHeaderSize;
        };


    } // namespace internal


    // -------------------------------------------------------------------------
    template <typename T, typename U, typename I, typename O>
    inline MediaAsset<T, U, I, O>::MediaAsset(const std::string& name) : Asset<T, I, O>(name),
        mHeaderSize(0)
    {
    }

    template <typename T, typename U, typename I, typename O>
    inline U MediaAsset<T, U, I, O>::GetUserData(size_t size) const
    {
        return U(size);
    }

    // -------------------------------------------------------------------------
    template <typename T, typename U, typename I, typename O>
    inline VirtualFileFactory::istream_t MediaAsset<T, U, I, O>::OnAdjustDataStream(VirtualFileFactory::istream_t istream)
    {
        // for media asset types, we always will create ostream, regardless if istream is valid or not
        // because, we always want to trigger Asset::onFinshedLoad signal.
        if (VirtualFileFactory::iostream_t iostream = NewStream())
        {
            size_t size = istream ? GetIStreamSize(*istream) : 0;
            U userData = GetUserData(size);

            {
                internal::HeaderMarker headerMarker(mHeaderSize, *iostream);
                // \note: list of all extra data we want to pass before the
                // actual blob of data. Any data added here must be accounted
                // for in load and save archive methods
                CopyArchiveValue(userData, *iostream);
            }

            if (istream)
            {
                CopyStream(*istream, *iostream);
            }

            return iostream;
        }

        return istream;
    }

    // -------------------------------------------------------------------------
    template <typename T, typename U, typename I, typename O>
    inline VirtualFileFactory::ostream_t MediaAsset<T, U, I, O>::OnAdjustDataStream(VirtualFileFactory::ostream_t ostream)
    {
        if (ostream)
        {
            if (VirtualFileFactory::iostream_t iostream = NewStream(internal::StreamDeleter(ostream, mHeaderSize)))
            {
                return iostream;
            }
        }

        return ostream;
    }

    // -------------------------------------------------------------------------
    template <typename T, typename U, typename I, typename O>
    template<class Archive>
    inline void MediaAsset<T, U, I, O>::save(Archive & ar, const unsigned int /*version*/) const
    {
        uint8_t *pDataBuffer = 0;
        size_t size = 0;

        SaveBlob(pDataBuffer, size);
        boost::scoped_array<uint8_t> dataBuffer(pDataBuffer);

        U userData = GetUserData(size);
        ar & userData;
        if (size)
        {
            ar.save_binary(pDataBuffer, size);
        }
    }

    // -------------------------------------------------------------------------
    template <typename T, typename U, typename I, typename O>
    template<class Archive>
    inline void MediaAsset<T, U, I, O>::load(Archive & ar, const unsigned int /*version*/)
    {
        U userData;
        ar & userData;
        if (userData.Size)
        {
            std::vector<uint8_t> dataBuffer(userData.Size);
            ar.load_binary(&dataBuffer[0], userData.Size);

            LoadBlob(&dataBuffer[0], userData.Size, streamName());
        }
    }


} // namespace eg

#endif // MEDIA_MEDIA_ASSET_IMPL_H


//=====================================================================
// Copyright 2016 (c), Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
//=====================================================================

#include "cpImageLoader.h"
#include <ImfArray.h>
#include "cExr.h"
#include "qdebug.h"
#include "Textureio.h"
#include "CmdLine.h"

bool g_useCPUDecode = true;
MIPIMAGE_FORMAT g_gpudecodeFormat = Format_OpenGL;

extern PluginManager g_pluginManager;
extern MipSet* DecompressMIPSet(MipSet *MipSetIn, CMP_GPUDecode decodeWith, bool useCPU, bool swizzle = false);
extern QRgb RgbaToQrgba(struct Imf::Rgba imagePixel);
extern int    g_OpenGLMajorVersion;

CImageLoader::CImageLoader()
{
    m_CMips = new CMIPS();
    m_pluginManager = &g_pluginManager;
}

CImageLoader::CImageLoader(void *plugin)
{
    m_CMips = new CMIPS();
    m_pluginManager = (PluginManager *) plugin;
}


CImageLoader::~CImageLoader()
{
    if (m_CMips)
    {
        delete m_CMips;
        m_CMips = NULL;
    }
}

CMipImages::CMipImages()
{
    // Init pointers
    mipset = NULL;
    Image_list.clear();
    MIPS2QtFailed = false;
    m_MipImageFormat = MIPIMAGE_FORMAT::Format_QImage;
    m_Error = MIPIMAGE_FORMAT_ERRORS::Format_NoErrors;
    m_DecompressedFormat = MIPIMAGE_FORMAT_DECOMPRESSED::Format_NONE;
    decompressedMipSet = NULL;
}


// Deletes all allocated CMipImage data
bool CImageLoader::clearMipImages(CMipImages *MipImages)
{
    if (!MipImages) return false;

    try
    {
        //if (MipImages->m_MipImageFormat == MIPIMAGE_FORMAT::Format_QImage)
        //{
            for (int i = 0; i < MipImages->Image_list.count(); i++)
            {
                QImage *image = MipImages->Image_list[i];
                if (image)
                {
                    delete image;
                    image = NULL;
                }
            }

            MipImages->Image_list.clear();
        //}

        if (MipImages->mipset)
            m_CMips->FreeMipSet(MipImages->mipset);

        if (MipImages->decompressedMipSet)
            m_CMips->FreeMipSet(MipImages->decompressedMipSet);

        delete MipImages;
        MipImages = NULL;


    }
    catch (...)
    {
        return false;
    }
    return true;
}


void CImageLoader::QImageFormatInfo(QImage *image)
{
    // QImage info
    int numofbitsperpixel = image->depth();
    int bytecount = image->byteCount();
    int bytesPerLine = image->bytesPerLine();
    int height = image->height();
    int numBytes = image->byteCount();
    int numColors = image->colorCount();
    QPixelFormat pixelFormat = image->pixelFormat();
    bool hasAlphaChannel = image->hasAlphaChannel();
    int width = image->width();
    QImage::Format format = image->format();

    // Stop compile messages for unused variables
    Q_UNUSED(numofbitsperpixel);
    Q_UNUSED(bytecount);
    Q_UNUSED(bytesPerLine);
    Q_UNUSED(height);
    Q_UNUSED(numBytes);
    Q_UNUSED(numColors);
    Q_UNUSED(pixelFormat);
    Q_UNUSED(hasAlphaChannel);
    Q_UNUSED(width);
    Q_UNUSED(format);
}


//load ARGB32 Qimage format to Mips
MipSet *CImageLoader::QImage2MIPS(QImage *qimage)
{
    if (qimage == NULL)
    {
        return NULL;
    }

    // QImage info for debugging
    // QImageFormatInfo(qimage);

    // Check supported format
    if (!(  (qimage->format() == QImage::Format_ARGB32) || 
            (qimage->format() == QImage::Format_ARGB32_Premultiplied) ||
            (qimage->format() == QImage::Format_RGB32)))
    {
        return NULL;
    }

    MipSet *pMipSet;
    pMipSet = new MipSet();
    if (pMipSet == NULL)
        return (NULL);
    memset(pMipSet, 0, sizeof(MipSet));



    // Set the channel formats and mip levels
    pMipSet->m_ChannelFormat    = CF_8bit;
    pMipSet->m_TextureDataType  = TDT_ARGB;
    pMipSet->m_dwFourCC         = 0;
    pMipSet->m_dwFourCC2        = 0;
    pMipSet->m_TextureType      = TT_2D;


    // Allocate default MipSet header
    m_CMips->AllocateMipSet(pMipSet,
                            pMipSet->m_ChannelFormat,
                            pMipSet->m_TextureDataType,
                            pMipSet->m_TextureType,
                            qimage->width(), 
                            qimage->height(), 
                            1);
    
    // Determin buffer size and set Mip Set Levels we want to use for now
    MipLevel *mipLevel = m_CMips->GetMipLevel(pMipSet, 0);
    pMipSet->m_nMipLevels = 1;
    m_CMips->AllocateMipLevelData(mipLevel, pMipSet->m_nWidth, pMipSet->m_nHeight, pMipSet->m_ChannelFormat, pMipSet->m_TextureDataType);

    // We have allocated a data buffer to fill get its referance
    BYTE* pData = (BYTE*)(mipLevel->m_pbData);

    QRgb qRGB;
    int i = 0;
    for (int y = 0; y < qimage->height(); y++){
        for (int x = 0; x < qimage->width();x++){
            qRGB = qimage->pixel(x, y);
            pData[i] = qRed(qRGB);
            i++;
            pData[i] = qGreen(qRGB);
            i++;
            pData[i] = qBlue(qRGB);
            i++;
            pData[i] = qAlpha(qRGB);
            i++;
        }
    }

    //pMipSet->m_pcData = cdata; ???

    if (pMipSet->m_format == CMP_FORMAT_Unknown)
    {
        pMipSet->m_format = QFormat2MipFormat(qimage->format());
    }


    return pMipSet;
}

// Finds a matching Qt Image format for the Mip Set
// Qt V5.4 has
// Format_Mono,
// Format_MonoLSB,
// Format_Indexed8,
// Format_RGB32,
// Format_ARGB32,
// Format_ARGB32_Premultiplied,
// Format_RGB16,
// Format_ARGB8565_Premultiplied,
// Format_RGB666,
// Format_ARGB6666_Premultiplied,
// Format_RGB555,
// Format_ARGB8555_Premultiplied,
// Format_RGB888,
// Format_RGB444,
// Format_ARGB4444_Premultiplied,
// Format_RGBX8888,
// Format_RGBA8888,
// Format_RGBA8888_Premultiplied
// Format_BGR30,
// Format_A2BGR30_Premultiplied,
// Format_RGB30,
// Format_A2RGB30_Premultiplied,
//
QImage::Format CImageLoader::MipFormat2QFormat(MipSet *mipset)
{
    QImage::Format format = QImage::Format_Invalid;

    switch (mipset->m_ChannelFormat)
    {
        case CF_8bit        : {format = QImage::Format_ARGB32; break; }
        case CF_Float16     : {format = QImage::Format_ARGB32; break; }
        case CF_Float32     : {format = QImage::Format_ARGB32; break; }
        case CF_Compressed  : {break;}
        case CF_16bit       : {break;}
        case CF_2101010     : {break;}
        case CF_32bit       : {format = QImage::Format_ARGB32;  break;}
        default             : {break;}
    }

    return format;
}



CMP_FORMAT CImageLoader::QFormat2MipFormat(QImage::Format qformat)
{
    CMP_FORMAT format = CMP_FORMAT_Unknown;

    switch (qformat)
    {

    case QImage::Format_RGB32:
    
    // Swizzed
    case QImage::Format_RGBX8888:
    case QImage::Format_RGBA8888:
    case QImage::Format_RGBA8888_Premultiplied:

    // Not Swizzed
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        format = CMP_FORMAT_ARGB_8888;
         break;

    case QImage::Format_RGB888:
        format = CMP_FORMAT_RGB_888;
        break;

    case QImage::Format_Invalid:
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
    case QImage::Format_Indexed8:
    case QImage::Format_RGB16:
    case QImage::Format_ARGB8565_Premultiplied:
    case QImage::Format_RGB666:
    case QImage::Format_ARGB6666_Premultiplied:
    case QImage::Format_RGB555:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_RGB444:
    case QImage::Format_ARGB4444_Premultiplied:
    case QImage::Format_BGR30:
    case QImage::Format_A2BGR30_Premultiplied:
    case QImage::Format_RGB30:
    case QImage::Format_A2RGB30_Premultiplied:
    case QImage::Format_Alpha8:
    case QImage::Format_Grayscale8:
    default:
        format = CMP_FORMAT_Unknown;
        break;
    }

    return format;
}

MipSet *CImageLoader::DecompressMipSet(CMipImages *MipImages)
{
    MipSet *tmpMipSet             = NULL;
    MipImages->decompressedMipSet = NULL;
    ;
    if ((MipImages->mipset->m_compressed) || (CompressedFormat(MipImages->mipset->m_format)))
    {
        //=======================================================
        // We use CPU based decode if OpenGL is not at or above 
        // V4.0
        //=======================================================
        if (g_useCPUDecode)
        {
            MipImages->m_DecompressedFormat = MIPIMAGE_FORMAT_DECOMPRESSED::Format_CPU;

            // This call decompresses all MIP levels so it should only be called once
            tmpMipSet = DecompressMIPSet(MipImages->mipset, GPUDecode_INVALID, true);

            if (tmpMipSet)
            {
                //---------------------------
                // swizzle Decompressed Data!
                //---------------------------
                if (KeepSwizzle(MipImages->mipset->m_format))
                {
                    SwizzleMipMap(tmpMipSet);
                }

                tmpMipSet->m_isDeCompressed = MipImages->mipset->m_format != CMP_FORMAT_Unknown? MipImages->mipset->m_format:CMP_FORMAT_MAX;
                MipImages->m_MipImageFormat = MIPIMAGE_FORMAT::Format_QImage;
                MipImages->decompressedMipSet = tmpMipSet;
            }
        }

        if (tmpMipSet == NULL)
        {
            if (!g_useCPUDecode)
            {
                MipImages->m_DecompressedFormat = MIPIMAGE_FORMAT_DECOMPRESSED::Format_GPU;
                MipImages->m_MipImageFormat     = g_gpudecodeFormat;
                
                CMP_GPUDecode decodeWith = CMP_GPUDecode::GPUDecode_INVALID;
                switch (g_gpudecodeFormat)
                {
                    case MIPIMAGE_FORMAT::Format_OpenGL:
                        decodeWith = CMP_GPUDecode::GPUDecode_OPENGL;
                        break;
                    case MIPIMAGE_FORMAT::Format_DirectX:
                        decodeWith = CMP_GPUDecode::GPUDecode_DIRECTX;
                        break;
                    case MIPIMAGE_FORMAT::Format_Vulkan:
                        decodeWith = CMP_GPUDecode::GPUDecode_VULKAN;
                        break;
                    default:
                        break;
                }

                // This call decompresses all MIP levels so it should only be called once
                tmpMipSet = DecompressMIPSet(MipImages->mipset, decodeWith, false);

                if (tmpMipSet)
                {
                    tmpMipSet->m_isDeCompressed     = MipImages->mipset->m_format != CMP_FORMAT_Unknown ? MipImages->mipset->m_format : CMP_FORMAT_MAX;
                    MipImages->decompressedMipSet   = tmpMipSet;
                }
            }
            else
                MipImages->m_Error              = MIPIMAGE_FORMAT_ERRORS::Format_CompressedImage;    
            //Reserved: GPUDecode           
        }
        else
            MipImages->m_DecompressedFormat = MIPIMAGE_FORMAT_DECOMPRESSED::Format_CPU;
    }
    else
    {
        tmpMipSet = MipImages->mipset;
        if (!(tmpMipSet->m_compressed))
            tmpMipSet->m_isDeCompressed = tmpMipSet->m_format;
    }

    return tmpMipSet;
}



//load data byte in mipset into Qimage ARGB32 format
QImage *CImageLoader::MIPS2QImage(MipSet *tmpMipSet, int level)
{
    if (tmpMipSet == NULL)
    {
        QImage *image = new QImage(":/CompressonatorGUI/Images/CompressedImageError.png");
        return image;
    }

    MipLevel* mipLevel = m_CMips->GetMipLevel(tmpMipSet, level);
    if (!mipLevel)
    {
        return nullptr;
    }

    QImage *image;
    
    if (
        (tmpMipSet->m_TextureDataType == TDT_ARGB) ||
        (tmpMipSet->m_TextureDataType == TDT_XRGB)
        )
   {

        if (tmpMipSet->m_ChannelFormat == CF_Float32)
        {
            Array2D<Rgba> pixels(tmpMipSet->m_nHeight, tmpMipSet->m_nWidth);
            float *data = mipLevel->m_pfData;
            if (data == NULL)  return nullptr;

            image = new QImage(mipLevel->m_nWidth, mipLevel->m_nHeight, MipFormat2QFormat(tmpMipSet));
            if (image == NULL)
            {
                image = new QImage(":/CompressonatorGUI/Images/OutOfMemoryError.png");
                return nullptr;
            }

            if (tmpMipSet->m_isDeCompressed != CMP_FORMAT_Unknown)
            {
                float r, g, b, a;
                //copy pixels into image
                for (int y = 0; y < mipLevel->m_nHeight; y++) {
                    for (int x = 0; x < mipLevel->m_nWidth; x++) {

                        //  Step 1) Compensate for fogging by subtracting defog
                        //     from the raw pixel values.
                        //  with defog of 0.0, this is a no-op

                        //  Step 2) Multiply the defogged pixel values by
                        //     2^(exposure + 2.47393).
                        // (2^2.47393) is 5.55555
                        r = *data * 5.55555;
                        data++;
                        g = *data * 5.55555;
                        data++;
                        b = *data * 5.55555;
                        data++;
                        // For Float channel formats Alpha channel is ignored
                        // Please update this as needed for specific image formats
                        a = 16777216;
                        data++;
                        image->setPixel(x, y, floatToQrgba(r, g, b, a));
                    }
                }
            }
            else
            {
                float r, g, b, a;

                //copy pixels into image
                for (int y = 0; y < mipLevel->m_nHeight; y++) {
                    for (int x = 0; x < mipLevel->m_nWidth; x++) {

                        //  Step 1) Compensate for fogging by subtracting defog
                        //     from the raw pixel values.
                        //  with defog of 0.0, this is a no-op

                        //  Step 2) Multiply the defogged pixel values by
                        //     2^(exposure + 2.47393).
                        // (2^2.47393) is 5.55555
 
                        r = half_conv_float((unsigned short)(*data)) * 5.55555;
                        data++;
                        g = half_conv_float((unsigned short)(*data)) * 5.55555;
                        data++;
                        b = half_conv_float((unsigned short)(*data)) * 5.55555;
                        data++;
                        a = *data;
                        data++;
                        image->setPixel(x, y, floatToQrgba(r, g, b, a));
                    }
                }
            }

        }
        else
        {
        
            // We have allocated a data buffer to fill get its referance
            BYTE* pData = mipLevel->m_pbData;
            if (pData == NULL)  return nullptr;
            
            // We dont support the conversion 
            if (MipFormat2QFormat(tmpMipSet) == QImage::Format_Invalid)
            {
                return nullptr;
            }

            // Allocates a uninitialized buffer of specified size and format
            image = new QImage(mipLevel->m_nWidth, mipLevel->m_nHeight, MipFormat2QFormat(tmpMipSet));
            if (image == NULL)
            {
                image = new QImage(":/CompressonatorGUI/Images/OutOfMemoryError.png");
                return nullptr;
            }

            QImageFormatInfo(image);

            // Initialize the buffer
            BYTE R, G, B, A;
            int i = 0;
            for (int y = 0; y < mipLevel->m_nHeight; y++){
                for (int x = 0; x < mipLevel->m_nWidth; x++)
                {
                    R = pData[i];
                    i++;
                    G = pData[i];
                    i++;
                    B = pData[i];
                    i++;
                    if (R==G && R==B)
                        A = 255;
                    else
                        A = pData[i];
                    i++;
                    image->setPixel(x, y, qRgba(R, G, B, A));
                }
            }
        }
    }

    return image;
}


MipSet * CImageLoader::LoadPluginMIPS(QString filename)
{
    QFileInfo fi(filename.toUpper());
    QString name = fi.fileName();
    QStringList list1 = name.split(".");
    QString PlugInType = list1[list1.size() - 1];
    QByteArray ba = PlugInType.toLatin1();
    const char *Ext = ba.data();

    PluginInterface_Image *plugin_Image;
    plugin_Image = reinterpret_cast<PluginInterface_Image *>(m_pluginManager->GetPlugin("IMAGE", (char *)Ext));
    //============================
    // AMD supported file formats 
    //============================
    if (plugin_Image)
    {
       MipSet *pMipSet;
       pMipSet = new MipSet();
       if (pMipSet == NULL)
              return (NULL);

       memset(pMipSet, 0, sizeof(MipSet));

       // for our loading of PNG, BMP etc...
       // We should check file extensions
       // and maybe improve the loader so it always
       // return RGBA formated data
       //pMipSet->m_swizzle = true;

       plugin_Image->TC_PluginSetSharedIO(m_CMips);

       QByteArray array = filename.toLocal8Bit();
       char* pFileNamePath = array.data();

       if (plugin_Image->TC_PluginFileLoadTexture(pFileNamePath, pMipSet) != 0)
        {
            // Process Error
            if (plugin_Image)
                delete plugin_Image;
            if (pMipSet)
                delete pMipSet;
            return (NULL);
        }

        if (plugin_Image)
            delete plugin_Image;
        
        plugin_Image = NULL;

        // bug fix 
        if (pMipSet->m_ChannelFormat == CF_Compressed)
            pMipSet->m_compressed = true;

        return (pMipSet);
    }
    else
        return (NULL);
}

// 
// Scans to match MIP levels with Generated Images
//
void CImageLoader::UpdateMIPMapImages(CMipImages *MipImages)
{
    if (!MipImages->mipset) return;
    QImage *image;

    //if (MipImages->m_MipImageFormat == MIPIMAGE_FORMAT::Format_QImage)
    //{
        if (MipImages->Image_list.count() <= MipImages->mipset->m_nMipLevels)
        {
            for (int i = 1; i <= MipImages->mipset->m_nMipLevels; i++)
            {
                if (MipImages->decompressedMipSet)
                    image = MIPS2QImage(MipImages->decompressedMipSet, i);
                else
                    image = MIPS2QImage(MipImages->mipset, i);

                if (image)
                {
                    MipImages->Image_list.append(image);
                }
            }
        }
    //}
}



CMipImages * CImageLoader::LoadPluginImage(QString filename)
{
    CMipImages *MipImages;
    QImage     *image = NULL;
    bool        usedQT = false;

    MipImages = new CMipImages();
    if (MipImages == NULL) return (NULL);
    MipImages->mipset = NULL;

    QFile file(filename);
    if (!file.exists())
    {
        MipImages->m_Error = MIPIMAGE_FORMAT_ERRORS::Format_InvalidFile;
        image = new QImage(":/CompressonatorGUI/Images/ImageFileDoesNotExist.png");
        usedQT = true;
    }

    // -------------------------------------------------------------------------
    // Try Our Plugins First to handle special cases of ASTC, DDS, KTX, EXR etc...
    // -------------------------------------------------------------------------

    QFileInfo fi(filename);
    QString ext = fi.suffix().toUpper();
    bool useAMD_Plugin = true;

    // -------------------------------------------------------
    // Exception on load as DDS for BCn < 6 is not working
    // Enable this to force loading via Qt
    // useAMD_Plugin = (ext.compare("DDS") != 0);
    // -------------------------------------------------------

    if (useAMD_Plugin)
    {
        MipImages->mipset = LoadPluginMIPS(filename);
        if (MipImages->mipset)
        {
            // Check Image Format is valid else try setting one 
            // based on a FourCC value
            if (MipImages->mipset->m_format == CMP_FORMAT_Unknown)
            {
                MipImages->mipset->m_format = GetFormat(MipImages->mipset);
            }

            MipSet *tmpMipSet;
            tmpMipSet = DecompressMipSet(MipImages);

            if (filename.contains(".exr") || filename.contains(".EXR"))
                tmpMipSet->m_isDeCompressed = CMP_FORMAT_Unknown;

            if (tmpMipSet == NULL)
            {
                image  = new QImage(":/CompressonatorGUI/Images/DeCompressImageError.png");
                usedQT = true;
            }
            else
                image = MIPS2QImage(tmpMipSet, 0);
        }
    }


    //------------------------------------------------------------------------------
    // Now try to load an image that can be viewed as QImage if not already loaded
    //------------------------------------------------------------------------------

    if (image == NULL)
    {
        image = new QImage(filename);
        usedQT = true;
    }


    //-----------------------------------------
    // Do we have a Image if so keep it
    //-----------------------------------------

    if (image) 
    {
        // validate the format is not compressed!
        QImage::Format  format = image->format();
        if (format != QImage::Format_Invalid)
        {
            MipImages->Image_list.append(image);

            if (MipImages->mipset)
            {
                if (!MipImages->mipset->m_compressed)
                    MipImages->m_MipImageFormat = MIPIMAGE_FORMAT::Format_QImage;

                if (usedQT)
                {
                    MipImages->mipset->m_format = QFormat2MipFormat(format);
                }

            }
            else
                MipImages->m_MipImageFormat = MIPIMAGE_FORMAT::Format_QImage;

        }
        else
        {
            // We dont want to use invalid QImage formats
            delete image;
            image = NULL;
        }
    }

    //---------------------------------------------------------------------------------
    // Failed to create a MipSet and we have a QImage, convert the QImage to a MIP set
    //---------------------------------------------------------------------------------

    if ((MipImages->Image_list.count() > 0) && (MipImages->mipset == NULL))
    {
        MipImages->mipset = QImage2MIPS(MipImages->Image_list[0]);
    }

    //-----------------------------------------
    // Update the images for all MIP levels
    //-----------------------------------------

    if (MipImages->mipset)
    {
        if (MipImages->mipset->m_nMipLevels > 1)
            UpdateMIPMapImages(MipImages);
    }


    //-----------------------------------------------------
    // Error : Both Qt and AMD failed to load an Image
    //-----------------------------------------------------

    if ((MipImages->mipset == NULL) && (MipImages->Image_list.count() == 0))
    {
        MipImages->m_Error = MIPIMAGE_FORMAT_ERRORS::Format_NotSupported;
        // we have a bug to fix!!
        QImage *image = new QImage(":/CompressonatorGUI/Images/notsupportedImage.png");
        if (image)
        {
            MipImages->Image_list.append(image);
            MipImages->m_MipImageFormat = MIPIMAGE_FORMAT::Format_QImage;
        }
    }

    return MipImages;
}


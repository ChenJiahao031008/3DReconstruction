//
// Created by caoqi on 2018/8/27.
//

#include "defines.h"
#include "functions.h"
#include "util/file_system.h"
#include "core/image_io.h"

#include <iostream>

core::ByteImage::Ptr
load_8bit_image (std::string const& fname, std::string* exif)
{
    std::string lcfname(util::string::lowercase(fname));
    std::string ext4 = util::string::right(lcfname, 4);
    std::string ext5 = util::string::right(lcfname, 5);
    try
    {
        if (ext4 == ".jpg" || ext5 == ".jpeg")
            return core::image::load_jpg_file(fname, exif);
        else if (ext4 == ".png" ||  ext4 == ".ppm"
                 || ext4 == ".tif" || ext5 == ".tiff")
            return core::image::load_file(fname);
    }
    catch (...)
    { }

    return core::ByteImage::Ptr();
}

core::RawImage::Ptr
load_16bit_image (std::string const& fname)
{
    std::string lcfname(util::string::lowercase(fname));
    std::string ext4 = util::string::right(lcfname, 4);
    std::string ext5 = util::string::right(lcfname, 5);
    try
    {
        if (ext4 == ".tif" || ext5 == ".tiff")
            return core::image::load_tiff_16_file(fname);
        else if (ext4 == ".ppm")
            return core::image::load_ppm_16_file(fname);
    }
    catch (...)
    { }

    return core::RawImage::Ptr();
}


core::FloatImage::Ptr
load_float_image (std::string const& fname)
{
    std::string lcfname(util::string::lowercase(fname));
    std::string ext4 = util::string::right(lcfname, 4);
    try
    {
        if (ext4 == ".pfm")
            return core::image::load_pfm_file(fname);
    }
    catch (...)
    { }

    return core::FloatImage::Ptr();
}


core::ImageBase::Ptr
load_any_image (std::string const& fname, std::string* exif)
{
    core::ByteImage::Ptr img_8 = load_8bit_image(fname, exif);
    if (img_8 != nullptr)
        return img_8;

    core::RawImage::Ptr img_16 = load_16bit_image(fname);
    if (img_16 != nullptr)
        return img_16;

    core::FloatImage::Ptr img_float = load_float_image(fname);
    if (img_float != nullptr)
        return img_float;


    std::cout << "Skipping file " << util::fs::basename(fname)
              << ", cannot load image." << std::endl;
    return core::ImageBase::Ptr();
}

std::string
remove_file_extension (std::string const& filename)
{
    std::size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos)
        return filename.substr(0, pos);
    return filename;
}


core::ImageBase::Ptr
limit_image_size (core::ImageBase::Ptr image, int max_pixels)
{
    switch (image->get_type())
    {
        case core::IMAGE_TYPE_FLOAT:
            return limit_image_size<float>(std::dynamic_pointer_cast
                    <core::FloatImage>(image), max_pixels);
        case core::IMAGE_TYPE_UINT8:
            return limit_image_size<uint8_t>(std::dynamic_pointer_cast
                    <core::ByteImage>(image), max_pixels);
        case core::IMAGE_TYPE_UINT16:
            return limit_image_size<uint16_t>(std::dynamic_pointer_cast
                    <core::RawImage>(image), max_pixels);
        default:
            break;
    }
    return core::ImageBase::Ptr();
}

bool
has_jpeg_extension (std::string const& filename)
{
    std::string lcfname(util::string::lowercase(filename));
    return util::string::right(lcfname, 4) == ".jpg"
           || util::string::right(lcfname, 5) == ".jpeg";
}

/* ---------------------------------------------------------------- */

std::string
make_image_name (int id)
{
    return "view_" + util::string::get_filled(id, 4) + ".mve";
}


void
add_exif_to_view (core::View::Ptr view, std::string const& exif)
{
    if (exif.empty())
        return;

    core::ByteImage::Ptr exif_image = core::ByteImage::create(exif.size(), 1, 1);
    std::copy(exif.begin(), exif.end(), exif_image->begin());
    view->set_blob(exif_image, "exif");
}

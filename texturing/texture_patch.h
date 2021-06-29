/*
 * Copyright (C) 2015, Nils Moehrle
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef TEX_TEXTUREPATCH_HEADER
#define TEX_TEXTUREPATCH_HEADER

#include <vector>

#include <math/vector.h>
#include <core/mesh.h>

#include "tri.h"
#include "poisson_blending.h"

int const texture_patch_border = 1;

/**
  * Class representing a texture patch.
  * Contains additionaly to the rectangular part of the TextureView
  * the faces which it textures and their relative texture coordinates.
  */
class TexturePatch {
    public:
        typedef std::shared_ptr<TexturePatch> Ptr;
        typedef std::shared_ptr<const TexturePatch> ConstPtr;
        typedef std::vector<std::size_t> Faces;     //facets
        typedef std::vector<math::Vec2f> Texcoords; // texcoords of vertices in Faces

    private:
        int label; // label of texture patch
        Faces faces; // facets
        Texcoords texcoords; // texcoords of each facet in Faces
        core::FloatImage::Ptr image;  // image
        core::ByteImage::Ptr validity_mask;
        core::ByteImage::Ptr blending_mask;

    public:
        /** Constructs a texture patch. */
        TexturePatch(int _label, std::vector<std::size_t> const & _faces,
            std::vector<math::Vec2f>  const & _texcoords, core::ByteImage::Ptr _image);

        TexturePatch(TexturePatch const & texture_patch);

        static TexturePatch::Ptr create(TexturePatch::ConstPtr texture_patch);
        static TexturePatch::Ptr create(int label, std::vector<std::size_t> const & faces,
            std::vector<math::Vec2f> const & texcoords, core::ByteImage::Ptr image);

        TexturePatch::Ptr duplicate(void);

        /** Adjust the image colors and update validity mask. */
        void adjust_colors(std::vector<math::Vec3f> const & adjust_values);

        /** Compares texture patches image size. */
        bool operator<(TexturePatch const & texture_patch) const;

        math::Vec3f get_pixel_value(math::Vec2f pixel) const;
        void set_pixel_value(math::Vec2i pixel, math::Vec3f color);

        bool valid_pixel(math::Vec2i pixel) const;
        bool valid_pixel(math::Vec2f pixel) const;

        std::vector<std::size_t> & get_faces(void);
        std::vector<std::size_t> const & get_faces(void) const;
        std::vector<math::Vec2f> & get_texcoords(void);
        std::vector<math::Vec2f> const & get_texcoords(void) const;

        core::FloatImage::ConstPtr get_image(void) const;
        core::ByteImage::ConstPtr get_validity_mask(void) const;
        core::ByteImage::ConstPtr get_blending_mask(void) const;

        std::pair<float, float> get_min_max(void) const;

        void release_blending_mask(void);
        void prepare_blending_mask(std::size_t strip_width);

        void erode_validity_mask(void);

        void blend(core::FloatImage::ConstPtr orig);

        int get_label(void) const;
        int get_width(void) const;
        int get_height(void) const;
        int get_size(void) const;
};

inline TexturePatch::Ptr
TexturePatch::create(TexturePatch::ConstPtr texture_patch) {
    return Ptr(new TexturePatch(*texture_patch));
}

inline TexturePatch::Ptr
TexturePatch::create(int label, std::vector<std::size_t> const & faces,
    std::vector<math::Vec2f>  const & texcoords, core::ByteImage::Ptr image) {
    return Ptr(new TexturePatch(label, faces, texcoords, image));
}

inline TexturePatch::Ptr
TexturePatch::duplicate(void) {
    return Ptr(new TexturePatch(*this));
}

inline int
TexturePatch::get_label(void) const {
    return label;
}

inline int
TexturePatch::get_width(void) const {
    return image->width();
}

inline int
TexturePatch::get_height(void) const {
    return image->height();
}

inline core::FloatImage::ConstPtr
TexturePatch::get_image(void) const {
    return image;
}

inline core::ByteImage::ConstPtr
TexturePatch::get_validity_mask(void) const {
    return validity_mask;
}

inline core::ByteImage::ConstPtr
TexturePatch::get_blending_mask(void) const {
    assert(blending_mask != NULL);
    return blending_mask;
}

inline void
TexturePatch::release_blending_mask(void) {
    assert(blending_mask != NULL);
    blending_mask.reset();
}

inline std::vector<math::Vec2f> &
TexturePatch::get_texcoords(void) {
    return texcoords;
}

inline std::vector<std::size_t> &
TexturePatch::get_faces(void) {
    return faces;
}

inline std::vector<math::Vec2f> const &
TexturePatch::get_texcoords(void) const {
    return texcoords;
}

inline std::vector<std::size_t> const &
TexturePatch::get_faces(void) const {
    return faces;
}

inline int
TexturePatch::get_size(void) const {
    return get_width() * get_height();
}

inline bool
TexturePatch::operator<(TexturePatch const & other) const {
    return get_size() < other.get_size();
}

#endif /* TEX_TEXTUREPATCH_HEADER */

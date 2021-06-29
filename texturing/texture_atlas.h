/*
 * Copyright (C) 2015, Nils Moehrle
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef TEX_TEXTUREATLAS_HEADER
#define TEX_TEXTUREATLAS_HEADER


#include <vector>

#include <util/exception.h>
#include <math/vector.h>
#include <core/mesh.h>
#include <core/image.h>

#include "tri.h"
#include "texture_patch.h"
#include "rectangular_bin.h"

/**
  * Class representing a texture atlas.
  */
class TextureAtlas {
    public:
        typedef std::shared_ptr<TextureAtlas> Ptr;

        typedef std::vector<std::size_t> Faces;
        typedef std::vector<std::size_t> TexcoordIds;
        typedef std::vector<math::Vec2f> Texcoords;

    private:
        unsigned int const size;
        unsigned int const padding;
        bool finalized;

        Faces faces;
        Texcoords texcoords;
        TexcoordIds texcoord_ids;

        core::ByteImage::Ptr image;
        core::ByteImage::Ptr validity_mask;

        RectangularBin::Ptr bin;

        std::string filename;

        void apply_edge_padding(void);
        void merge_texcoords(void);

    public:
        /** Constructs a texture patch. */
        TextureAtlas(unsigned int size);
        ~TextureAtlas(void);

        static TextureAtlas::Ptr create(unsigned int size);

        Faces const & get_faces(void) const;
        TexcoordIds const & get_texcoord_ids(void) const;
        Texcoords const & get_texcoords(void) const;
        std::string const & get_filename(void) const;

        bool insert(TexturePatch::ConstPtr texture_patch,
            float vmin, float vmax);

        void finalize(void);
};

inline TextureAtlas::Ptr
TextureAtlas::create(unsigned int size) {
    return Ptr(new TextureAtlas(size));
}

inline TextureAtlas::Faces const &
TextureAtlas::get_faces(void) const {
    return faces;
}

inline TextureAtlas::TexcoordIds const &
TextureAtlas::get_texcoord_ids(void) const {
    return texcoord_ids;
}

inline TextureAtlas::Texcoords const &
TextureAtlas::get_texcoords(void) const {
    return texcoords;
}

inline std::string const &
TextureAtlas::get_filename(void) const {
    if (!finalized) {
        throw util::Exception("Texture atlas not finalized");
    }
    return filename;
}

#endif /* TEX_TEXTUREATLAS_HEADER */

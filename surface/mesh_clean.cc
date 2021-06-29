/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include "math/defines.h"
#include "core/mesh.h"
#include "core/mesh_tools.h"
#include "core/mesh_info.h"
#include "surface/mesh_clean.h"

FSSR_NAMESPACE_BEGIN

bool
edge_collapse (core::TriangleMesh::Ptr mesh, core::VertexInfoList& vinfos,
    std::size_t v1, std::size_t v2, math::Vec3f const& new_vert,
    std::vector<std::size_t> const& afaces,
    float acos_threshold = 0.95f)
{
    core::TriangleMesh::FaceList& faces = mesh->get_faces();
    core::TriangleMesh::VertexList& verts = mesh->get_vertices();

    /* Test if the hypothetical vertex destroys geometry. */
    core::MeshVertexInfo& vinfo1 = vinfos[v1];
    for (std::size_t i = 0; i < vinfo1.verts.size(); ++i)
    {
        std::size_t ip1 = (i + 1) % vinfo1.verts.size();
        if (vinfo1.verts[i] == v2 || vinfo1.verts[ip1] == v2)
            continue;

        math::Vec3f const& av1 = verts[vinfo1.verts[i]];
        math::Vec3f const& av2 = verts[vinfo1.verts[ip1]];
        math::Vec3f n1 = (av1 - verts[v1]).cross(av2 - verts[v1]).normalized();
        math::Vec3f n2 = (av1 - new_vert).cross(av2 - new_vert).normalized();

        float dot = n1.dot(n2);
        if (MATH_ISNAN(dot) || dot < acos_threshold)
            return false;
    }

    core::MeshVertexInfo& vinfo2 = vinfos[v2];
    for (std::size_t i = 0; i < vinfo2.verts.size(); ++i)
    {
        std::size_t ip1 = (i + 1) % vinfo2.verts.size();
        if (vinfo2.verts[i] == v1 || vinfo2.verts[ip1] == v1)
            continue;
        math::Vec3f const& av1 = verts[vinfo2.verts[i]];
        math::Vec3f const& av2 = verts[vinfo2.verts[ip1]];
        math::Vec3f n1 = (av1 - verts[v2]).cross(av2 - verts[v2]).normalized();
        math::Vec3f n2 = (av1 - new_vert).cross(av2 - new_vert).normalized();

        float dot = n1.dot(n2);
        if (MATH_ISNAN(dot) || dot < acos_threshold)
            return false;
    }

    /* Test succeeded. Assign new vertex position to v1. */
    verts[v1] = new_vert;

    /* Update faces adjacent to v2 replacing v2 with v1. */
    for (std::size_t i = 0; i < vinfo2.faces.size(); ++i)
        for (std::size_t j = 0; j < 3; ++j)
            if (faces[vinfo2.faces[i] * 3 + j] == v2)
                faces[vinfo2.faces[i] * 3 + j] = v1;

    /* Delete the two faces adjacent to the collapsed edge. */
    std::size_t v3 = 0, v4 = 0;
    for (std::size_t i = 0; i < 3; ++i)
    {
        std::size_t fid1 = afaces[0] * 3 + i;
        std::size_t fid2 = afaces[1] * 3 + i;
        if (faces[fid1] != v1 && faces[fid1] != v2)
            v3 = faces[fid1];
        if (faces[fid2] != v1 && faces[fid2] != v2)
            v4 = faces[fid2];
        faces[fid1] = 0;
        faces[fid2] = 0;
    }

    /* Update vertex info for vertices adjcent to v2, replacing v2 with v1. */
    for (std::size_t i = 0; i < vinfo2.verts.size(); ++i)
    {
        std::size_t const vert_id = vinfo2.verts[i];
        if (vert_id != v1 && vert_id != v3 && vert_id != v4)
            vinfos[vert_id].replace_adjacent_vertex(v2, v1);
    }

    /* Update vertex info for v3 and v4: remove v2, remove deleted faces. */
    core::MeshVertexInfo& vinfo3 = vinfos[v3];
    vinfo3.remove_adjacent_face(afaces[0]);
    vinfo3.remove_adjacent_vertex(v2);
    core::MeshVertexInfo& vinfo4 = vinfos[v4];
    vinfo4.remove_adjacent_face(afaces[1]);
    vinfo4.remove_adjacent_vertex(v2);

    /* Update vinfo for v1: Remove v2, remove collapsed faces, add v2 faces. */
    vinfo1.remove_adjacent_face(afaces[0]);
    vinfo1.remove_adjacent_face(afaces[1]);
    for (std::size_t i = 0; i < vinfo2.faces.size(); ++i)
    if (vinfo2.faces[i] != afaces[0] && vinfo2.faces[i] != afaces[1])
            vinfo1.faces.push_back(vinfo2.faces[i]);
    vinfos.order_and_classify(*mesh, v1);

    /* Update vertex info for v2. */
    vinfo2.faces.clear();
    vinfo2.verts.clear();
    vinfo2.vclass = core::VERTEX_CLASS_UNREF;

    return true;
}

/* ---------------------------------------------------------------- */

namespace
{
    /*
     * Returns the ratio of the smallest by the second smallest edge length.
     */
    float
    get_needle_ratio_squared (core::TriangleMesh::VertexList const& verts,
        unsigned int const* vid,
        std::size_t* shortest_edge_v1, std::size_t* shortest_edge_v2)
    {
        typedef std::pair<float, int> Edge;
        Edge edges[3];
        for (int j = 0; j < 3; ++j)
        {
            int const jp1 = (j + 1) % 3;
            edges[j].first = (verts[vid[j]] - verts[vid[jp1]]).square_norm();
            edges[j].second = j;
        }
        math::algo::sort_values(edges + 0, edges + 1, edges + 2);

        /* Test shortest to second-shortest edge ratio. */
        float const square_ratio = edges[0].first / edges[1].first;
        if (shortest_edge_v1 != nullptr && shortest_edge_v2 != nullptr)
        {
            *shortest_edge_v1 = vid[edges[0].second];
            *shortest_edge_v2 = vid[(edges[0].second + 1) % 3];
        }

        return square_ratio;
    }
}

std::size_t
clean_needles (core::TriangleMesh::Ptr mesh, float needle_ratio_thres)
{
    float const square_needle_ratio_thres = MATH_POW2(needle_ratio_thres);
    core::VertexInfoList vinfos(mesh);

    /*
     * Algorithm to remove slivers with a two long and a very short edge.
     * The sliver is identified using the ratio of the shortest by the second
     * shortest edge. An edge collapse of the short edge is performed if it
     * does not modify the geometry in a negative way, e.g. flips triangles.
     */
    core::TriangleMesh::FaceList& faces = mesh->get_faces();
    core::TriangleMesh::VertexList& verts = mesh->get_vertices();
    std::size_t num_collapses = 0;

    for (std::size_t i = 0; i < faces.size(); i += 3)
    {
        /* Skip invalid faces. */
        if (faces[i] == faces[i + 1] && faces[i] == faces[i + 2])
            continue;

        /* Skip faces that are no needles. */
        std::size_t v1, v2;
        float const needle_ratio_squared
            = get_needle_ratio_squared(verts, &faces[i], &v1, &v2);
        if (needle_ratio_squared > square_needle_ratio_thres)
            continue;

        /* Skip edges between non-simple vertices. */
        if (vinfos[v1].vclass != core::VERTEX_CLASS_SIMPLE
            || vinfos[v2].vclass != core::VERTEX_CLASS_SIMPLE)
            continue;

        /* Find triangle adjecent to the edge, skip non-simple edges. */
        std::vector<std::size_t> afaces;
        vinfos.get_faces_for_edge(v1, v2, &afaces);
        if (afaces.size() != 2)
            continue;

        /* Collapse the edge. */
        math::Vec3f new_v = (verts[v1] + verts[v2]) / 2.0f;
        if (edge_collapse(mesh, vinfos, v1, v2, new_v, afaces))
            num_collapses += 1;
    }

    /* Cleanup invalid triangles and unreferenced vertices. */
    core::geom::mesh_delete_unreferenced(mesh);

    return num_collapses;
}

/* ---------------------------------------------------------------- */

std::size_t
clean_caps (core::TriangleMesh::Ptr mesh)
{
    core::VertexInfoList vinfos(mesh);
    core::TriangleMesh::VertexList& verts = mesh->get_vertices();
    std::size_t num_collapses = 0;
    for (std::size_t v1 = 0; v1 < verts.size(); ++v1)
    {
        core::MeshVertexInfo& vinfo = vinfos[v1];

        if (vinfo.vclass != core::VERTEX_CLASS_SIMPLE)
            continue;

        if (vinfo.verts.size() != 3)
            continue;

        std::pair<float, std::size_t> edge_len[3];
        for (std::size_t j = 0; j < vinfo.verts.size(); ++j)
            edge_len[j] = std::make_pair(
                (verts[vinfo.verts[j]] - verts[v1]).square_norm(),
                vinfo.verts[j]);
        math::algo::sort_values(edge_len + 0, edge_len + 1, edge_len + 2);
        std::size_t v2 = edge_len[0].second;

        std::vector<std::size_t> afaces;
        vinfos.get_faces_for_edge(v1, v2, &afaces);
        if (afaces.size() != 2)
            continue;

        /* Edge collapse fails if (v2 - v1) is not coplanar to triangle. */
        if (edge_collapse(mesh, vinfos, v1, v2, verts[v2], afaces))
            num_collapses += 1;
    }

    /* Cleanup invalid triangles and unreferenced vertices. */
    core::geom::mesh_delete_unreferenced(mesh);

    return num_collapses;
}

/* ---------------------------------------------------------------- */

std::size_t
clean_mc_mesh (core::TriangleMesh::Ptr mesh, float needle_ratio_thres)
{
    std::size_t num_collapsed = 0;
    num_collapsed += clean_needles(mesh, needle_ratio_thres);
    num_collapsed += clean_caps(mesh);
    num_collapsed += clean_needles(mesh, needle_ratio_thres);
    return num_collapsed;
}

FSSR_NAMESPACE_END

#include "drake/geometry/render/gl_renderer/dev/load_mesh.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <tiny_obj_loader.h>

#include "drake/common/drake_assert.h"

namespace drake {
namespace geometry {
namespace render {
namespace internal {

using std::string;
using std::vector;

std::pair<VertexBuffer, IndexBuffer> LoadMeshFromObj(
    std::istream* input_stream) {
  tinyobj::attrib_t attrib;
  vector<tinyobj::shape_t> shapes;
  vector<tinyobj::material_t> materials;
  string err;
  // This renderer assumes everything is triangles -- we rely on tinyobj to
  // triangulate for us.
  const bool do_tinyobj_triangulation = true;

  // Tinyobj doesn't infer the search directory from the directory containing
  // the obj file. We have to provide that directory; of course, this assumes
  // that the material library reference is relative to the obj directory.
  // Ignore material-library file.
  tinyobj::MaterialReader* material_reader = nullptr;
  const bool ret =
      tinyobj::LoadObj(&attrib, &shapes, &materials, &err, input_stream,
                       material_reader, do_tinyobj_triangulation);
  // As of tinyobj v1.0.6, we expect that `ret` will *always* be true. We are
  // capturing it and asserting it so that if the version advances, and false is
  // ever returned, CI will inform us so we can update the error messages.
  DRAKE_DEMAND(ret == true);

  if (shapes.size() == 0) {
    throw std::runtime_error(
        "The OBJ data appears to have no faces; it could be missing faces or "
        "might not be an OBJ file");
  }

  DRAKE_DEMAND(shapes.size() > 0);
  // Accumulate vertices.
  const vector<tinyobj::real_t>& verts = attrib.vertices;
  const int v_count = static_cast<int>(verts.size()) / 3;
  DRAKE_DEMAND(static_cast<int>(verts.size()) == v_count * 3);
  VertexBuffer vertices{v_count, 3};
  for (int v = 0; v < v_count; ++v) {
    const int i = v * 3;
    vertices.block<1, 3>(v, 0) << verts[i], verts[i + 1], verts[i + 2];
  }

  // Accumulate faces.
  int tri_count = 0;
  for (const auto& shape : shapes) {
    const tinyobj::mesh_t& raw_mesh = shape.mesh;
    DRAKE_DEMAND(raw_mesh.indices.size() ==
                 raw_mesh.num_face_vertices.size() * 3);
    tri_count += raw_mesh.num_face_vertices.size();
  }

  IndexBuffer indices{tri_count, 3};
  int tri_index = 0;
  for (const auto& shape : shapes) {
    const tinyobj::mesh_t& raw_mesh = shape.mesh;
    for (int sub_index = 0;
         sub_index < static_cast<int>(raw_mesh.num_face_vertices.size());
         ++sub_index) {
      const int i = sub_index * 3;
      indices.block<1, 3>(tri_index, 0)
          << static_cast<GLuint>(raw_mesh.indices[i].vertex_index),
          static_cast<GLuint>(raw_mesh.indices[i + 1].vertex_index),
          static_cast<GLuint>(raw_mesh.indices[i + 2].vertex_index);
      ++tri_index;
    }
  }
  return std::make_pair(vertices, indices);
}

std::pair<VertexBuffer, IndexBuffer> LoadMeshFromObj(const string& filename) {
  std::ifstream input_stream(filename);
  if (!input_stream.is_open()) {
    throw std::runtime_error(
        fmt::format("Cannot load the obj file '{}'", filename));
  }
  return LoadMeshFromObj(&input_stream);
}

}  // namespace internal
}  // namespace render
}  // namespace geometry
}  // namespace drake

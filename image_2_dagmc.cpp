#include <iostream>
#include <cmath>
#include "MBTagConventions.hpp"
#include "moab/Core.hpp"
#include "moab/Types.hpp"
#include "moab/EntityType.hpp"
#include "moab/Skinner.hpp"
#include "moab/GeomTopoTool.hpp"

moab::Core *mbi;
moab::Skinner *skinner;
moab::GeomTopoTool *gtt;

const char geom_categories[][CATEGORY_TAG_SIZE] = {"Vertex\0",
						   "Curve\0",
						   "Surface\0",
						   "Volume\0",
						   "Group\0"};

moab::Tag geometry_dimension_tag;
moab::Tag id_tag;
moab::Tag faceting_tol_tag;
moab::Tag geometry_resabs_tag;
moab::Tag category_tag;
moab::Tag name_tag;

void setup_tags() {
  moab::ErrorCode rval = moab::MB_FAILURE;
  rval = mbi->tag_get_handle(GEOM_DIMENSION_TAG_NAME, 1, moab::MB_TYPE_INTEGER, geometry_dimension_tag, moab::MB_TAG_DENSE | moab::MB_TAG_CREAT);
  MB_CHK_SET_ERR_RET(rval, "Couldnt get geom dim tag");

  rval = mbi->tag_get_handle(GLOBAL_ID_TAG_NAME, 1, moab::MB_TYPE_INTEGER, id_tag, moab::MB_TAG_DENSE | moab::MB_TAG_CREAT);
  MB_CHK_SET_ERR_RET(rval, "Couldnt get id tag");
    
  rval = mbi->tag_get_handle("FACETING_TOL", 1, moab::MB_TYPE_DOUBLE, faceting_tol_tag,
			     moab::MB_TAG_SPARSE | moab::MB_TAG_CREAT);
  MB_CHK_SET_ERR_RET(rval, "Error creating faceting_tol_tag");

  rval = mbi->tag_get_handle("GEOMETRY_RESABS", 1, moab::MB_TYPE_DOUBLE, 
			     geometry_resabs_tag, moab::MB_TAG_SPARSE | moab::MB_TAG_CREAT);
  MB_CHK_SET_ERR_RET(rval, "Error creating geometry_resabs_tag");
    
  rval = mbi->tag_get_handle(CATEGORY_TAG_NAME, CATEGORY_TAG_SIZE, moab::MB_TYPE_OPAQUE, 
			     category_tag, moab::MB_TAG_SPARSE | moab::MB_TAG_CREAT);
  MB_CHK_SET_ERR_RET(rval, "Error creating category_tag");

  rval = mbi->tag_get_handle(NAME_TAG_NAME, NAME_TAG_SIZE, moab::MB_TYPE_OPAQUE,
			     name_tag, moab::MB_TAG_SPARSE | moab::MB_TAG_CREAT);
  
  return;
}

void set_surface_tags(int id, moab::EntityHandle surface_set) {
  const void* id_val = &id;
  moab::ErrorCode rval;

  int val_s = 2;
  const void* val_surf = &val_s;
  rval = mbi->tag_set_data(category_tag,&surface_set,1,&geom_categories[2]);
  rval = mbi->tag_set_data(geometry_dimension_tag,&surface_set,1,val_surf);
  rval = mbi->tag_set_data(id_tag,&surface_set,1,id_val);
  return;
}

int get_smallest_free_surf_id() {
  moab::ErrorCode rval = moab::MB_FAILURE;
  const void* vals[] = {geom_categories[2]}; 
  moab::Range surface_sets;
  rval = mbi->get_entities_by_type_and_tag(0,moab::MBENTITYSET,
							   &category_tag,vals,1,
							   surface_sets);

  std::vector<int> data(surface_sets.size());

  rval = mbi->tag_get_data(id_tag,surface_sets,&data[0]);
  int surf_id = 0;
  for ( int i = 0 ; i < surface_sets.size() ; i++ ) {
    if ( data[i] > surf_id ) surf_id = data[i];
  }
  surf_id++;
  return surf_id;
}

void set_volume_tags(int id, moab::EntityHandle volume_set) {
  const void* id_val = &id;
  moab::ErrorCode rval;

  int val_v = 3;
  const void* val_vol = &val_v;
  rval = mbi->tag_set_data(category_tag,&volume_set,1,&geom_categories[3]);
  rval = mbi->tag_set_data(geometry_dimension_tag,&volume_set,1,val_vol);
  rval = mbi->tag_set_data(id_tag,&volume_set,1,id_val);
  return;
}

void set_group_tags(int id, std::string name, moab::EntityHandle group_set) {
  const void* id_val = &id;
  moab::ErrorCode rval;

  int val_g = 4;
  const void* val_group = &val_g;
  std::string group_name;
  if ( name == "" ) {
    group_name = "mat:M"+std::to_string(id);
  } else {
    group_name = "mat:"+name;
  }
  
  char namebuf[NAME_TAG_SIZE];
  memset(namebuf,'\0', NAME_TAG_SIZE); // fill with null
  strncpy(namebuf,group_name.c_str(),NAME_TAG_SIZE-1);
  // = {group_name.c_str()};
  rval = mbi->tag_set_data(name_tag,&group_set,1,namebuf);
  rval = mbi->tag_set_data(category_tag,&group_set,1,&geom_categories[4]);
  rval = mbi->tag_set_data(geometry_dimension_tag,&group_set,1,val_group);
  rval = mbi->tag_set_data(id_tag,&group_set,1,id_val);
  return;
}

// make a new surface from the triangles in the range, remove those triangles
// from their original parent, set the appropriate sense 
moab::EntityHandle make_new_surface(moab::Range triangles, 
                                    const moab::EntityHandle parent_surface) {

  moab::EntityHandle surface_set; // new surface
  moab::ErrorCode rval = moab::MB_FAILURE;
  // make new surface set
  rval = mbi->create_meshset(moab::MESHSET_SET,surface_set);
  // set the surface tags
  int id = get_smallest_free_surf_id();
  set_surface_tags(id,surface_set);
  // remove the triangles from their parent
  rval = mbi->remove_entities(parent_surface,triangles);
  // and add them to their child
  rval = mbi->add_entities(surface_set,triangles);
  // find the parent volume of the original surface
  moab::Range volumes;
  rval = mbi->get_parent_meshsets(parent_surface,volumes);
  if(volumes.size() != 1 ) {
    std::cout << "Volume has too many parents" << std::endl;
  }
  // add the surface set as another child of the volume
  rval = mbi->add_parent_child(volumes[0],surface_set);
  rval = gtt->set_sense(surface_set,volumes[0],1);

  return surface_set;
}

// given the range of reversed entities find which surface sets
// they belonged to
std::vector<moab::EntityHandle> get_surfaces(moab::Range reversed) {
  // long way - find all the surface sets, loop over each, check
  // the ranges for overlaps, if so remove from range and
  std::vector<moab::EntityHandle> surfs;
  const void* vals[] = {geom_categories[2]}; 
  moab::Range surface_sets;
  moab::ErrorCode rval = mbi->get_entities_by_type_and_tag(0,moab::MBENTITYSET,
							   &category_tag,vals,1,
							   surface_sets);

  if(surface_sets.size() == 0) {
    std::cout << "ERROR: surface sets returned has size 0" << std::endl;
    exit(0);
  }
  
  // loop over the surfaces
  for ( moab::EntityHandle surf : surface_sets ) {
    moab::Range triangles;
    // get all the triangles in the surface
    rval = mbi->get_entities_by_type(surf,moab::MBTRI,triangles);
    // subtract the reversed set from the triangles set
    moab::Range overlap = intersect(triangles,reversed);
    // the overlap contains the triangles that are both reversed
    // and a member of triangles
    if( overlap.size() > 0 ) {
      // also need to make a new surface that contains the overlap
      // triangles - and remove them from surface 
      moab::EntityHandle new_surf = make_new_surface(overlap,surf);
      surfs.push_back(new_surf);
    }
  }
  
  return surfs;
}

void build_geom(double tag_value, moab::Tag tag) {
  moab::ErrorCode rval;
  std::cout << "Skinning " << tag_value << " ..." << std::endl;
  moab::Range tets;
  double value = tag_value;
  const void* vals[] = {&value};
  rval = mbi->get_entities_by_type_and_tag(0,moab::MBTET,
					   &tag,
					   vals,1,tets);
  moab::Range surface;
  moab::Range reversed; // reversed are the tries with reverse sense from skin
  moab::Range all_tris; // all_tris are all those that come back from skinning
  
  rval = skinner->find_skin(0,tets,false,surface,&reversed);
  std::cout << surface.size() << " " << reversed.size() << std::endl;

  std::vector<moab::EntityHandle> surfaces; 
  
  if ( reversed.size() != 0 ) {
    // find the surfaces that belong to the triangles in reversed
    // 
    surfaces = get_surfaces(reversed);    
  }
  
  // this is the id of the volume and surface
  int id = (int) tag_value;
  
  // tag the surface set
  moab::EntityHandle surface_set;
  rval = mbi->create_meshset(moab::MESHSET_SET,surface_set);
  set_surface_tags(id,surface_set);
  
  // tag the volume set
  moab::EntityHandle volume_set;
  rval = mbi->create_meshset(moab::MESHSET_SET,volume_set);
  set_volume_tags(id,volume_set);
  
  // tag the group set
  moab::EntityHandle group_set;
  rval = mbi->create_meshset(moab::MESHSET_SET,group_set);
  set_group_tags(id,"",group_set);
		 
  // add parent child links
  rval = mbi->add_parent_child(volume_set,surface_set);
  rval = mbi->add_entities(group_set, &volume_set,1); // materials belong to set
  // add the triangles to the surface set 
  rval = mbi->add_entities(surface_set,surface);
  // need to set the surface senses
  //rval = gtt->set_surface_senses(surface_set,volume_set,NULL);
  rval = gtt->set_sense(surface_set,volume_set,1);

  // if there are any surfaces that already exist they must have 
  // reverse sense relative to their parent
  if (surfaces.size() > 0 ) {
    // for each surface 
    for ( moab::EntityHandle child_surface : surfaces ) {
      // add a new parent child link
      rval = mbi->add_parent_child(volume_set,child_surface);
      // they must have the reverse sense
      rval = gtt->set_sense(child_surface,volume_set,-1);
    }
  }
  
  return;
}

moab::ErrorCode make_tri(std::vector<moab::EntityHandle> vertices, int v1, int v2 , int v3, moab::Range &surface_tris) {
  moab::ErrorCode rval = moab::MB_SUCCESS;
  moab::EntityHandle triangle;
  moab::EntityHandle connectivity[3] = { vertices[v1],vertices[v2],vertices[v3] };
  rval = mbi->create_element(moab::MBTRI,connectivity,3,triangle);
  surface_tris.insert(triangle);

  return rval;
}

// build a cubit surface
moab::ErrorCode build_cube_surface(double pos[3], moab::EntityHandle surface, double factor, bool inside) {
   // we will have 8 vertices
   //pos[0],pos[1],pos[2]
   //pos[0],pos[1],-pos[2]
   //pos[0],-pos[1],pos[2]
   //pos[0],-pos[1],-pos[2]
   //-pos[0],pos[1],pos[2]
   //-pos[0],pos[1],-pos[2]
   //-pos[0],-pos[1],pos[2]
   //-pos[0],-pos[1],-pos[2]
  moab::ErrorCode rval = moab::MB_FAILURE;
  double coord[3] = {pos[0]*factor,pos[1]*factor,pos[2]*factor};
  std::vector<moab::EntityHandle> vertices;
  moab::EntityHandle vertex;
  // first four
  rval = mbi->create_vertex(coord,vertex);
  vertices.push_back(vertex);

  coord[2] *= -1.0;
  rval = mbi->create_vertex(coord,vertex);
  vertices.push_back(vertex);

  coord[1] *= -1.0;
  coord[2] *= -1.0;
  rval = mbi->create_vertex(coord,vertex);
  vertices.push_back(vertex);

  coord[2] *= -1.0;
  rval = mbi->create_vertex(coord,vertex);
  vertices.push_back(vertex);

  // creating second batch of four
  coord[0] = -1.0*pos[0]*factor;
  coord[1] = pos[1]*factor;
  coord[2] = pos[2]*factor;
  
  rval = mbi->create_vertex(coord,vertex);
  vertices.push_back(vertex);

  coord[2] *= -1.0;
  rval = mbi->create_vertex(coord,vertex);
  vertices.push_back(vertex);

  coord[1] *= -1.0;
  coord[2] *= -1.0;
  rval = mbi->create_vertex(coord,vertex);
  vertices.push_back(vertex);

  coord[2] *= -1.0;
  rval = mbi->create_vertex(coord,vertex);
  vertices.push_back(vertex);

  // now have 8 vertices to use to make surface
  moab::Range surface_tris;
  // note the node ordering is important - needs to be
  // anticlockwise for normals to point 'out'
  if ( ! inside ) {
    rval = make_tri(vertices,3,1,0,surface_tris);
    rval = make_tri(vertices,2,3,0,surface_tris);
    // top
    rval = make_tri(vertices,0,4,2,surface_tris);
    rval = make_tri(vertices,4,6,2,surface_tris);
    // side
    rval = make_tri(vertices,0,1,4,surface_tris);
    rval = make_tri(vertices,1,5,4,surface_tris);
    // front
    rval = make_tri(vertices,4,5,7,surface_tris);
    rval = make_tri(vertices,4,7,6,surface_tris);
    // bottom
    rval = make_tri(vertices,5,1,3,surface_tris);
    rval = make_tri(vertices,3,7,5,surface_tris);
    // side
    rval = make_tri(vertices,6,3,2,surface_tris);
    rval = make_tri(vertices,6,7,3,surface_tris);
  } else { // clockwise winding - normals pont in
    rval = make_tri(vertices,0,1,3,surface_tris);
    rval = make_tri(vertices,0,3,2,surface_tris);
    // top
    rval = make_tri(vertices,2,4,0,surface_tris);
    rval = make_tri(vertices,2,6,4,surface_tris);
    // side
    rval = make_tri(vertices,4,1,0,surface_tris);
    rval = make_tri(vertices,4,5,1,surface_tris);
    // front
    rval = make_tri(vertices,7,5,4,surface_tris);
    rval = make_tri(vertices,6,7,4,surface_tris);
    // bottom
    rval = make_tri(vertices,3,1,5,surface_tris);
    rval = make_tri(vertices,5,7,3,surface_tris);
    // side
    rval = make_tri(vertices,2,3,6,surface_tris);
    rval = make_tri(vertices,3,7,6,surface_tris);
  }
  // add the new triangles to the surface
  rval = mbi->add_entities(surface,surface_tris);
  
  return moab::MB_SUCCESS;
}

// build the graveyard
void build_graveyard(int id) {
  moab::ErrorCode rval = moab::MB_FAILURE;
  double maximum[3] = {-1e38,-1e38,-1e38};
  /*
  rval = gtt->construct_obb_trees();

  for ( int i = 1 ; i <= 3 ; i++ ) {
    moab::EntityHandle vol = gtt->entity_by_id(3,i);
    double min_p[3], max_p[3];
    rval = gtt->get_bounding_coords(vol,min_p,max_p);
    maximum[0] = std::max(std::abs(min_p[0]),std::abs(max_p[0]));
    maximum[1] = std::max(std::abs(min_p[1]),std::abs(max_p[1]));
    maximum[2] = std::max(std::abs(min_p[2]),std::abs(max_p[2]));
  }
  */
  maximum[0] = 100., maximum[1] = 100. , maximum[2] = 100.;
  // we now have max coords, build box
  moab::EntityHandle surface1,surface2;
  rval = mbi->create_meshset(moab::MESHSET_SET,surface1);
  MB_CHK_SET_ERR_RET(rval,"failed to create meshset");
  rval = mbi->create_meshset(moab::MESHSET_SET,surface2);
  MB_CHK_SET_ERR_RET(rval,"failed to create meshset");

  rval = build_cube_surface(maximum,surface1,1.0,true);
  MB_CHK_SET_ERR_RET(rval,"failed to build cube");
  rval = build_cube_surface(maximum,surface2,1.2,false);
  MB_CHK_SET_ERR_RET(rval,"failed to build cube");

  moab::EntityHandle volume;
  rval = mbi->create_meshset(moab::MESHSET_SET,volume);
  MB_CHK_SET_ERR_RET(rval,"failed to create meshset");
  rval = mbi->add_parent_child(volume, surface1);
  MB_CHK_SET_ERR_RET(rval,"failed to add parent child");
  rval = mbi->add_parent_child(volume, surface2);
  MB_CHK_SET_ERR_RET(rval,"failed to add parent child");

  set_surface_tags(get_smallest_free_surf_id(),surface1);
  set_surface_tags(get_smallest_free_surf_id(),surface2);
  set_volume_tags(id,volume);
  
  rval = gtt->set_sense(surface1,volume,1);
  rval = gtt->set_sense(surface2,volume,1);

  // tag the group set
  moab::EntityHandle group_set; 
  rval = mbi->create_meshset(moab::MESHSET_SET,group_set);
  set_group_tags(id,"Graveyard",group_set);

  rval = mbi->add_entities(group_set,&volume,1);
  
  return;
}

int main(int argc, char* argv[]) {
  mbi = new moab::Core();
  moab::ErrorCode rval;
  moab::EntityHandle file_set;

  gtt = new moab::GeomTopoTool(mbi,false);
  
  setup_tags();
  
  std::cout << "loading file..." << std::endl;
  rval = mbi->create_meshset(moab::MESHSET_TRACK_OWNER,file_set);
  rval = mbi->load_file("visit_ex_db.h5m", &file_set);

  double faceting_tol = 1.e-4;
  double geom_tol = 1.e-6;
  
  rval = mbi->tag_set_data(faceting_tol_tag, &file_set, 1, &faceting_tol);
  rval = mbi->tag_set_data(geometry_resabs_tag, &file_set, 1, &geom_tol);
  
  // get the tets only
  moab::Range tets,edge_tets;
  std::cout << "Getting tets..." << std::endl;
  rval = mbi->get_entities_by_type(file_set,moab::MBTET,tets);

  skinner = new moab::Skinner(mbi);

  moab::Tag material_tag;
  rval = mbi->tag_get_handle("Material", 1, moab::MB_TYPE_DOUBLE, material_tag, moab::MB_TAG_SPARSE | moab::MB_TAG_CREAT);

  
  build_geom(1.0,material_tag);
  build_geom(2.0,material_tag);
  build_geom(3.0,material_tag);
 
  build_graveyard(4);
  
  rval = mbi->delete_entities(tets);
  delete gtt;
  
  std::cout << "Writing mesh..." << std::endl;
  rval = mbi->write_mesh("edges_new.h5m");

  delete skinner;
  delete mbi;
  
  return 0;
  
}

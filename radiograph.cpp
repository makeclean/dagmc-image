#include <iostream>
#include <fstream>
#include "moab/Types.hpp"
#include "moab/Core.hpp"
#include "DagMC.hpp"

class hits {
  public:
  hits(){
    hitc = 0;
  }
  
  ~hits(){
    hit_collection.clear();
  }
  
  void add_hit(double pos[3]) {
    hitc++;
    std::array<double,3> hit_vec;
    hit_vec[0]=pos[0], hit_vec[1] = pos[1], hit_vec[2] = pos[2];
    hit_collection.push_back(hit_vec);
  }

  void write_vtk(std::string filename) {
    std::ofstream vtk;
    vtk.open(filename);
    vtk << "# vtk DataFile Version 2.0" << std::endl;
    vtk << "DAGMC Debug" << std::endl;
    vtk << "ASCII" << std::endl;
    vtk << "DATASET UNSTRUCTURED_GRID" << std::endl;
    vtk << "POINTS " << hitc << " float" << std::endl;
    for ( int i = 0 ; i < hitc ; i++ ) {
      vtk << hit_collection[i][0] << " ";
      vtk << hit_collection[i][1] << " ";
      vtk << hit_collection[i][2] << std::endl;
    }
    vtk << std::endl;
    vtk << "CELLS " << hitc-1 << " " << (hitc-1)*3 << std::endl;
    for ( int i = 0 ; i < hitc-1; i++ ) {
      vtk << "2 " << i << " " << i+1 << std::endl;
    }
    vtk << std::endl;
    vtk << "CELL_TYPES " << hitc-1 << std::endl;
    for ( int i = 0 ; i < hitc-1 ; i++ ) {
      vtk << "3" << std::endl;
    }
    vtk.close();
    return;
  }
  
  int hitc;
  std::vector<std::array<double,3> > hit_collection;
};

moab::EntityHandle find_cell(moab::DagMC *dag,double pos[3]) {
  moab::ErrorCode rval = moab::MB_FAILURE;
  moab::EntityHandle vol;
  int inside = 0;
  for ( int i = 1 ; i <= dag->num_entities(3) ; i++ ) {
    vol = dag->entity_by_index(3,i);
    rval = dag->point_in_volume(vol,pos,inside);
    if(inside){
      std::cout << "point in volume " << vol << std::endl;
      return vol;
    }
  }
  return 0;
}

class score {
   score();
  ~score();

  void add_score(const double pos[3], const double dir[3], double length) {
    double hit_pos[3];
    hit_pos[0] = pos[0] +
  }
  double x_width;
  double y_width;

};

int main(int argc, char* argv[]) {

  moab::DagMC *DAG = new moab::DagMC();

  moab::ErrorCode rval = moab::MB_FAILURE;

  std::string infile(argv[1]);
  
  rval = DAG->load_file(infile.c_str());

  rval = DAG->init_OBBTree();

  hits *hit = new hits();
  
  double pos[3] = {12.,2.3,7.8};
  //double pos[3] = {0.,0.,0.};
  hit->add_hit(pos);
  moab::EntityHandle vol = find_cell(DAG,pos);
  moab::EntityHandle new_vol;
  moab::EntityHandle next_surf;
  double dist;
  double dir[3] = {0,0,1};
  moab::DagMC::RayHistory hist;
  while(true){
    rval = DAG->ray_fire(vol,pos,dir,next_surf,dist,&hist);
    if(next_surf == 0) break;
    rval = DAG->next_vol(next_surf,vol,new_vol);
    std::cout << "next surf :" << DAG->get_entity_id(next_surf);
    std::cout << " curr_vol:" << DAG->get_entity_id(vol);
    std::cout << "  next_vol:" << DAG->get_entity_id(new_vol);
    std::cout << " dist:" << dist << std::endl;
    pos[0] += dir[0]*dist;
    pos[1] += dir[1]*dist;
    pos[2] += dir[2]*dist;
    std::cout << " pos: " << pos[0] << " " << pos[1] << " " << pos[2] << std::endl;
    hit->add_hit(pos);  
    vol = new_vol;
    next_surf = 0;
  }
  hit->write_vtk("hits.vtk");
  return 0;
}

//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y10 m06 d16

#include <sstream>
#include "common/common.h"
#include "file_io_ugx.h"
#include "common/parser/rapidxml/rapidxml_print.hpp"
#include "lib_grid/algorithms/attachment_util.h"

using namespace std;
using namespace rapidxml;

namespace ug
{

////////////////////////////////////////////////////////////////////////
bool SaveGridToUGX(Grid& grid, ISubsetHandler& sh,
				   const char* filename)
{
	if(grid.has_vertex_attachment(aPosition))
		return SaveGridToUGX(grid, sh, filename, aPosition);
	else if(grid.has_vertex_attachment(aPosition2))
		return SaveGridToUGX(grid, sh, filename, aPosition2);
	else if(grid.has_vertex_attachment(aPosition1))
		return SaveGridToUGX(grid, sh, filename, aPosition1);

	UG_LOG("ERROR in SaveGridToUGX: no standard attachment found.\n");
	return false;
}

bool LoadGridFromUGX(Grid& grid, ISubsetHandler& sh,
					const char* filename)
{
	if(grid.has_vertex_attachment(aPosition))
		return LoadGridFromUGX(grid, sh, filename, aPosition);
	else if(grid.has_vertex_attachment(aPosition2))
		return LoadGridFromUGX(grid, sh, filename, aPosition2);
	else if(grid.has_vertex_attachment(aPosition1))
		return LoadGridFromUGX(grid, sh, filename, aPosition1);

//	no standard position attachments are available.
//	Attach aPosition and use it.
	grid.attach_to_vertices(aPosition);
	return LoadGridFromUGX(grid, sh, filename, aPosition);
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//	GridWriterUGX
GridWriterUGX::GridWriterUGX()
{
	xml_node<>* decl = m_doc.allocate_node(node_declaration);
	decl->append_attribute(m_doc.allocate_attribute("version", "1.0"));
	decl->append_attribute(m_doc.allocate_attribute("encoding", "utf-8"));
	m_doc.append_node(decl);
}

GridWriterUGX::~GridWriterUGX()
{
//	detach aInt from the vertices of the grid
	for(size_t i = 0; i < m_vEntries.size(); ++i)
		m_vEntries[i].grid->detach_from_vertices(m_aInt);
}

bool GridWriterUGX::
write_to_stream(std::ostream& out)
{
	out << m_doc;
	return true;
}

bool GridWriterUGX::
write_to_file(const char* filename)
{
	ofstream out(filename);
	if(out){
		return write_to_stream(out);
	}
	return false;
}

void GridWriterUGX::
add_subset_attributes(rapidxml::xml_node<>* targetNode,
					  ISubsetHandler& sh, size_t subsetIndex)
{
	const SubsetInfo& si = sh.subset_info(subsetIndex);
//	write name
	targetNode->append_attribute(m_doc.allocate_attribute("name", si.name.c_str()));

//	write color
	{
		stringstream ss;
		for(size_t i = 0; i < 4; ++i){
			ss << si.color[i] << " ";
		}


	//	allocate a string and erase last character(' ')
		char* colorData = m_doc.allocate_string(ss.str().c_str(), ss.str().size() );
		colorData[ss.str().size() - 1] = 0;
		targetNode->append_attribute(m_doc.allocate_attribute("color", colorData));
	}
//	write state
	{
		stringstream ss;
		ss << (size_t)si.subsetState;
		char* stateData = m_doc.allocate_string(ss.str().c_str(), ss.str().size() + 1);
		stateData[ss.str().size()] = 0;
		targetNode->append_attribute(m_doc.allocate_attribute("state", stateData));
	}
}

void GridWriterUGX::
add_subset_handler(ISubsetHandler& sh, const char* name,
					size_t refGridIndex)
{
//	get the node of the referenced grid
	if(refGridIndex >= m_vEntries.size()){
		UG_LOG("GridWriterUGX::add_subset_handler: bad refGridIndex. Aborting.\n");
		return;
	}

	xml_node<>* parentNode = m_vEntries[refGridIndex].node;

//	create the subset-handler node
	xml_node<>* ndSH = m_doc.allocate_node(node_element, "subset_handler");
	ndSH->append_attribute(m_doc.allocate_attribute("name", name));

//	add the subset-handler-node to the grid-node.
	parentNode->append_node(ndSH);

//	add the subsets
	for(int i = 0; i < sh.num_subsets(); ++i){
		xml_node<>* ndSubset = m_doc.allocate_node(node_element, "subset");
		add_subset_attributes(ndSubset, sh, i);
		ndSH->append_node(ndSubset);

	//	add elements
		if(sh.contains_vertices(i))
			ndSubset->append_node(
				create_subset_element_node<VertexBase>("vertices", sh, i));
		if(sh.contains_edges(i))
			ndSubset->append_node(
				create_subset_element_node<EdgeBase>("edges", sh, i));
		if(sh.contains_faces(i))
			ndSubset->append_node(
				create_subset_element_node<Face>("faces", sh, i));
		if(sh.contains_volumes(i))
			ndSubset->append_node(
				create_subset_element_node<Volume>("volumes", sh, i));
	}
}

template <class TGeomObj>
rapidxml::xml_node<>* GridWriterUGX::
create_subset_element_node(const char* name, const ISubsetHandler& sh,
							size_t si)
{

//	the stringstream to which we'll write the data
	stringstream ss;

	if(sh.grid()){
	//	access the grid
		Grid& grid = *sh.grid();

	//	access the attachment
		Grid::AttachmentAccessor<TGeomObj, AInt> aaInd(grid, m_aInt);
		if(aaInd.valid()){
			GridObjectCollection goc = sh.get_grid_objects_in_subset(si);
			for(size_t lvl = 0; lvl < goc.num_levels(); ++lvl){
				for(typename geometry_traits<TGeomObj>::iterator iter =
					goc.begin<TGeomObj>(lvl); iter != goc.end<TGeomObj>(lvl); ++iter)
				{
					ss << aaInd[*iter] << " ";
				}
			}
		}
	}

	if(ss.str().size() > 0){
	//	allocate a string and erase last character(' ')
		char* nodeData = m_doc.allocate_string(ss.str().c_str(), ss.str().size());
		nodeData[ss.str().size()-1] = 0;
	//	create and return the node
		return m_doc.allocate_node(node_element, name, nodeData);
	}
	else{
	//	return an emtpy node
		return m_doc.allocate_node(node_element, name);
	}
}

template <class TGeomObj>
rapidxml::xml_node<>* GridWriterUGX::
create_selector_element_node(const char* name, const ISelector& sel)
{
//	the stringstream to which we'll write the data
	stringstream ss;

	if(sel.grid()){
	//	access the grid
		Grid& grid = *sel.grid();

	//	access the attachment
		Grid::AttachmentAccessor<TGeomObj, AInt> aaInd(grid, m_aInt);
		if(aaInd.valid()){
			GridObjectCollection goc = sel.get_grid_objects();
			for(size_t lvl = 0; lvl < goc.num_levels(); ++lvl){
				for(typename geometry_traits<TGeomObj>::iterator iter =
					goc.begin<TGeomObj>(lvl); iter != goc.end<TGeomObj>(lvl); ++iter)
				{
					ss << aaInd[*iter] << " " << (int)sel.get_selection_status(*iter) << " ";
				}
			}
		}
	}

	if(ss.str().size() > 0){
	//	allocate a string and erase last character(' ')
		char* nodeData = m_doc.allocate_string(ss.str().c_str(), ss.str().size());
		nodeData[ss.str().size()-1] = 0;
	//	create and return the node
		return m_doc.allocate_node(node_element, name, nodeData);
	}
	else{
	//	return an emtpy node
		return m_doc.allocate_node(node_element, name);
	}
}

void GridWriterUGX::
add_selector(ISelector& sel, const char* name, size_t refGridIndex)
{
//	get the node of the referenced grid
	if(refGridIndex >= m_vEntries.size()){
		UG_LOG("GridWriterUGX::add_selector: bad refGridIndex. Aborting.\n");
		return;
	}

	xml_node<>* parentNode = m_vEntries[refGridIndex].node;

//	create the selector node
	xml_node<>* ndSel = m_doc.allocate_node(node_element, "selector");
	ndSel->append_attribute(m_doc.allocate_attribute("name", name));

//	add the selector node to the grid-node.
	parentNode->append_node(ndSel);

//	add elements
	if(sel.contains_vertices())
		ndSel->append_node(create_selector_element_node<VertexBase>("vertices", sel));
	if(sel.contains_edges())
		ndSel->append_node(create_selector_element_node<EdgeBase>("edges", sel));
	if(sel.contains_faces())
		ndSel->append_node(create_selector_element_node<Face>("faces", sel));
	if(sel.contains_volumes())
		ndSel->append_node(create_selector_element_node<Volume>("volumes", sel));
}


void GridWriterUGX::
init_grid_attachments(Grid& grid)
{
//	assign indices to the vertices, edges, faces and volumes
	grid.attach_to_vertices(m_aInt);
	grid.attach_to_edges(m_aInt);
	grid.attach_to_faces(m_aInt);
	grid.attach_to_volumes(m_aInt);

//	access and initialise indices
	Grid::VertexAttachmentAccessor<AInt> aaIndVRT(grid, m_aInt);
	Grid::EdgeAttachmentAccessor<AInt> aaIndEDGE(grid, m_aInt);
	Grid::FaceAttachmentAccessor<AInt> aaIndFACE(grid, m_aInt);
	Grid::VolumeAttachmentAccessor<AInt> aaIndVOL(grid, m_aInt);

	int baseInd = 0;
	AssignIndices(grid.begin<RegularVertex>(), grid.end<RegularVertex>(), aaIndVRT, baseInd);
	baseInd += grid.num<RegularVertex>();
	AssignIndices(grid.begin<ConstrainedVertex>(), grid.end<ConstrainedVertex>(),
				  aaIndVRT, baseInd);

	baseInd = 0;
	AssignIndices(grid.begin<Edge>(), grid.end<Edge>(), aaIndEDGE, baseInd);
	baseInd += grid.num<Edge>();
	AssignIndices(grid.begin<ConstrainingEdge>(), grid.end<ConstrainingEdge>(),
				  aaIndEDGE, baseInd);
	baseInd += grid.num<ConstrainingEdge>();
	AssignIndices(grid.begin<ConstrainedEdge>(), grid.end<ConstrainedEdge>(),
				  aaIndEDGE, baseInd);

	baseInd = 0;
	AssignIndices(grid.begin<Triangle>(), grid.end<Triangle>(), aaIndFACE, baseInd);
	baseInd += grid.num<Triangle>();
	AssignIndices(grid.begin<ConstrainingTriangle>(), grid.end<ConstrainingTriangle>(), aaIndFACE, baseInd);
	baseInd += grid.num<ConstrainingTriangle>();
	AssignIndices(grid.begin<ConstrainedTriangle>(), grid.end<ConstrainedTriangle>(), aaIndFACE, baseInd);
	baseInd += grid.num<ConstrainedTriangle>();
	
	AssignIndices(grid.begin<Quadrilateral>(), grid.end<Quadrilateral>(), aaIndFACE, baseInd);
	baseInd += grid.num<Quadrilateral>();
	AssignIndices(grid.begin<ConstrainingQuadrilateral>(), grid.end<ConstrainingQuadrilateral>(), aaIndFACE, baseInd);
	baseInd += grid.num<ConstrainingQuadrilateral>();
	AssignIndices(grid.begin<ConstrainedQuadrilateral>(), grid.end<ConstrainedQuadrilateral>(), aaIndFACE, baseInd);
	baseInd += grid.num<ConstrainedQuadrilateral>();
	
	AssignIndices(grid.begin<Volume>(), grid.end<Volume>(), aaIndVOL, 0);
}

void GridWriterUGX::
add_elements_to_node(rapidxml::xml_node<>* node,
					  Grid& grid)
{
//	access and initialise indices
	Grid::VertexAttachmentAccessor<AInt> aaIndVRT(grid, m_aInt);
	Grid::EdgeAttachmentAccessor<AInt> aaIndEDGE(grid, m_aInt);
	Grid::FaceAttachmentAccessor<AInt> aaIndFACE(grid, m_aInt);
	Grid::VolumeAttachmentAccessor<AInt> aaIndVOL(grid, m_aInt);

//	write edges
	if(grid.num<Edge>() > 0)
		node->append_node(create_edge_node(grid.begin<Edge>(),
										grid.end<Edge>(), aaIndVRT));

//	write constraining edges
	if(grid.num<ConstrainingEdge>() > 0)
		node->append_node(create_constraining_edge_node(
										grid.begin<ConstrainingEdge>(),
										grid.end<ConstrainingEdge>(), aaIndVRT));

//	write constrained edges
	if(grid.num<ConstrainedEdge>() > 0)
		node->append_node(create_constrained_edge_node(
										grid.begin<ConstrainedEdge>(),
										grid.end<ConstrainedEdge>(),
										aaIndVRT, aaIndEDGE, aaIndFACE));
//	write triangles
	if(grid.num<Triangle>() > 0)
		node->append_node(create_triangle_node(grid.begin<Triangle>(),
												grid.end<Triangle>(), aaIndVRT));

//	write constraining triangles
	if(grid.num<ConstrainingTriangle>() > 0)
		node->append_node(create_constraining_triangle_node(
												grid.begin<ConstrainingTriangle>(),
												grid.end<ConstrainingTriangle>(),
												aaIndVRT));

//	write constrained triangles
	if(grid.num<ConstrainedTriangle>() > 0)
		node->append_node(create_constrained_triangle_node(
												grid.begin<ConstrainedTriangle>(),
												grid.end<ConstrainedTriangle>(),
												aaIndVRT, aaIndFACE));												
//	write quadrilaterals
	if(grid.num<Quadrilateral>() > 0)
		node->append_node(create_quadrilateral_node(grid.begin<Quadrilateral>(),
													grid.end<Quadrilateral>(), aaIndVRT));

//	write constraining quadrilaterals
	if(grid.num<ConstrainingQuadrilateral>() > 0)
		node->append_node(create_constraining_quadrilateral_node(
												grid.begin<ConstrainingQuadrilateral>(),
												grid.end<ConstrainingQuadrilateral>(),
												aaIndVRT));

//	write constrained quadrilaterals
	if(grid.num<ConstrainedQuadrilateral>() > 0)
		node->append_node(create_constrained_quadrilateral_node(
												grid.begin<ConstrainedQuadrilateral>(),
												grid.end<ConstrainedQuadrilateral>(),
												aaIndVRT, aaIndFACE));
																									
//	write tetrahedrons
	if(grid.num<Tetrahedron>() > 0)
		node->append_node(create_tetrahedron_node(grid.begin<Tetrahedron>(),
													grid.end<Tetrahedron>(), aaIndVRT));

//	write hexahedrons
	if(grid.num<Hexahedron>() > 0)
		node->append_node(create_hexahedron_node(grid.begin<Hexahedron>(),
													grid.end<Hexahedron>(), aaIndVRT));

//	write prisms
	if(grid.num<Prism>() > 0)
		node->append_node(create_prism_node(grid.begin<Prism>(),
											grid.end<Prism>(), aaIndVRT));

//	write pyramids
	if(grid.num<Pyramid>() > 0)
		node->append_node(create_pyramid_node(grid.begin<Pyramid>(),
											  grid.end<Pyramid>(), aaIndVRT));
}

rapidxml::xml_node<>* GridWriterUGX::
create_edge_node(EdgeIterator edgesBegin,
				 EdgeIterator edgesEnd,
				 AAVrtIndex aaIndVRT)
{
//	write the elements to a temporary stream
	stringstream ss;
	for(EdgeIterator iter = edgesBegin; iter != edgesEnd; ++iter)
	{
		ss << aaIndVRT[(*iter)->vertex(0)] << " " << aaIndVRT[(*iter)->vertex(1)] << " ";
	}

	if(ss.str().size() > 0){
	//	allocate a string and erase last character(' ')
		char* nodeData = m_doc.allocate_string(ss.str().c_str(), ss.str().size());
		nodeData[ss.str().size()-1] = 0;
	//	create and return the node
		return m_doc.allocate_node(node_element, "edges", nodeData);
	}
	else{
	//	return an emtpy node
		return m_doc.allocate_node(node_element, "edges");
	}
}

rapidxml::xml_node<>* GridWriterUGX::
create_constraining_edge_node(ConstrainingEdgeIterator edgesBegin,
				 			  ConstrainingEdgeIterator edgesEnd,
							  AAVrtIndex aaIndVRT)
{
//	write the elements to a temporary stream
	stringstream ss;
	for(ConstrainingEdgeIterator iter = edgesBegin; iter != edgesEnd; ++iter)
	{
		ss << aaIndVRT[(*iter)->vertex(0)] << " " << aaIndVRT[(*iter)->vertex(1)] << " ";
	}

	if(ss.str().size() > 0){
	//	allocate a string and erase last character(' ')
		char* nodeData = m_doc.allocate_string(ss.str().c_str(), ss.str().size());
		nodeData[ss.str().size()-1] = 0;
	//	create and return the node
		return m_doc.allocate_node(node_element, "constraining_edges", nodeData);
	}
	else{
	//	return an emtpy node
		return m_doc.allocate_node(node_element, "constraining_edges");
	}
}

rapidxml::xml_node<>* GridWriterUGX::
create_constrained_edge_node(ConstrainedEdgeIterator edgesBegin,
							 ConstrainedEdgeIterator edgesEnd,
							 AAVrtIndex aaIndVRT,
							 AAEdgeIndex aaIndEDGE,
							 AAFaceIndex aaIndFACE)
{
//	write the elements to a temporary stream
	stringstream ss;
	for(ConstrainedEdgeIterator iter = edgesBegin; iter != edgesEnd; ++iter)
	{
	//	write endpoint indices
		ss << aaIndVRT[(*iter)->vertex(0)] << " " << aaIndVRT[(*iter)->vertex(1)] << " ";

	//	write index of associated constraining element
	//	codes:	-1: no constraining element
	//			0: vertex. index follows
	//			1: edge. index follows
	//			2: face. index follows
	//			3: volume. index follows
		EdgeBase* ce = dynamic_cast<EdgeBase*>((*iter)->get_constraining_object());
		Face* cf = dynamic_cast<Face*>((*iter)->get_constraining_object());
		if(ce)
			ss << "1 " << aaIndEDGE[ce] << " ";
		else if(cf)
			ss << "2 " << aaIndFACE[cf] << " ";
		else
			ss << "-1 ";
	}

	if(ss.str().size() > 0){
	//	allocate a string and erase last character(' ')
		char* nodeData = m_doc.allocate_string(ss.str().c_str(), ss.str().size());
		nodeData[ss.str().size()-1] = 0;
	//	create and return the node
		return m_doc.allocate_node(node_element, "constrained_edges", nodeData);
	}
	else{
	//	return an emtpy node
		return m_doc.allocate_node(node_element, "constrained_edges");
	}
}

rapidxml::xml_node<>* GridWriterUGX::
create_triangle_node(TriangleIterator trisBegin,
				 	 TriangleIterator trisEnd,
				 	 AAVrtIndex aaIndVRT)
{
//	write the elements to a temporary stream
	stringstream ss;
	for(TriangleIterator iter = trisBegin; iter != trisEnd; ++iter)
	{
		ss << aaIndVRT[(*iter)->vertex(0)] << " " << aaIndVRT[(*iter)->vertex(1)]
			<< " " << aaIndVRT[(*iter)->vertex(2)] << " " ;
	}

	if(ss.str().size() > 0){
	//	allocate a string and erase last character(' ')
		char* nodeData = m_doc.allocate_string(ss.str().c_str(), ss.str().size());
		nodeData[ss.str().size()-1] = 0;
	//	create and return the node
		return m_doc.allocate_node(node_element, "triangles", nodeData);
	}
	else{
	//	return an emtpy node
		return m_doc.allocate_node(node_element, "triangles");
	}
}

rapidxml::xml_node<>* GridWriterUGX::
create_constraining_triangle_node(ConstrainingTriangleIterator trisBegin,
								  ConstrainingTriangleIterator trisEnd,
								  AAVrtIndex aaIndVRT)
{
//	write the elements to a temporary stream
	stringstream ss;
	for(ConstrainingTriangleIterator iter = trisBegin; iter != trisEnd; ++iter)
	{
		ss << aaIndVRT[(*iter)->vertex(0)] << " " << aaIndVRT[(*iter)->vertex(1)]
			<< " " << aaIndVRT[(*iter)->vertex(2)] << " " ;
	}

	if(ss.str().size() > 0){
	//	allocate a string and erase last character(' ')
		char* nodeData = m_doc.allocate_string(ss.str().c_str(), ss.str().size());
		nodeData[ss.str().size()-1] = 0;
	//	create and return the node
		return m_doc.allocate_node(node_element, "constraining_triangles", nodeData);
	}
	else{
	//	return an emtpy node
		return m_doc.allocate_node(node_element, "constraining_triangles");
	}
}

rapidxml::xml_node<>* GridWriterUGX::
create_constrained_triangle_node(ConstrainedTriangleIterator trisBegin,
								 ConstrainedTriangleIterator trisEnd,
								 AAVrtIndex aaIndVRT,
								 AAFaceIndex aaIndFACE)
{
//	write the elements to a temporary stream
	stringstream ss;
	for(ConstrainedTriangleIterator iter = trisBegin; iter != trisEnd; ++iter)
	{
	//	write endpoint indices
		ss << aaIndVRT[(*iter)->vertex(0)] << " " << aaIndVRT[(*iter)->vertex(1)]
			<< " " << aaIndVRT[(*iter)->vertex(2)] << " " ;
	//	write index of associated constraining element
	//	codes:	-1: no constraining element
	//			0: vertex. index follows
	//			1: edge. index follows
	//			2: face. index follows
	//			3: volume. index follows
		Face* cf = dynamic_cast<Face*>((*iter)->get_constraining_object());
		if(cf)
			ss << "2 " << aaIndFACE[cf] << " ";
		else
			ss << "-1 ";
	}

	if(ss.str().size() > 0){
	//	allocate a string and erase last character(' ')
		char* nodeData = m_doc.allocate_string(ss.str().c_str(), ss.str().size());
		nodeData[ss.str().size()-1] = 0;
	//	create and return the node
		return m_doc.allocate_node(node_element, "constrained_triangles", nodeData);
	}
	else{
	//	return an emtpy node
		return m_doc.allocate_node(node_element, "constrained_triangles");
	}
}


rapidxml::xml_node<>* GridWriterUGX::
create_quadrilateral_node(QuadrilateralIterator quadsBegin,
						  QuadrilateralIterator quadsEnd,
						  AAVrtIndex aaIndVRT)
{
//	write the elements to a temporary stream
	stringstream ss;
	for(QuadrilateralIterator iter = quadsBegin; iter != quadsEnd; ++iter)
	{
		ss << aaIndVRT[(*iter)->vertex(0)] << " " << aaIndVRT[(*iter)->vertex(1)] << " "
			<< aaIndVRT[(*iter)->vertex(2)] << " " << aaIndVRT[(*iter)->vertex(3)] << " " ;
	}

	if(ss.str().size() > 0){
	//	allocate a string and erase last character(' ')
		char* nodeData = m_doc.allocate_string(ss.str().c_str(), ss.str().size());
		nodeData[ss.str().size()-1] = 0;
	//	create and return the node
		return m_doc.allocate_node(node_element, "quadrilaterals", nodeData);
	}
	else{
	//	return an emtpy node
		return m_doc.allocate_node(node_element, "quadrilaterals");
	}
}

rapidxml::xml_node<>* GridWriterUGX::
create_constraining_quadrilateral_node(ConstrainingQuadrilateralIterator quadsBegin,
									   ConstrainingQuadrilateralIterator quadsEnd,
									   AAVrtIndex aaIndVRT)
{
//	write the elements to a temporary stream
	stringstream ss;
	for(ConstrainingQuadrilateralIterator iter = quadsBegin; iter != quadsEnd; ++iter)
	{
		ss << aaIndVRT[(*iter)->vertex(0)] << " " << aaIndVRT[(*iter)->vertex(1)] << " "
			<< aaIndVRT[(*iter)->vertex(2)] << " " << aaIndVRT[(*iter)->vertex(3)] << " " ;
	}

	if(ss.str().size() > 0){
	//	allocate a string and erase last character(' ')
		char* nodeData = m_doc.allocate_string(ss.str().c_str(), ss.str().size());
		nodeData[ss.str().size()-1] = 0;
	//	create and return the node
		return m_doc.allocate_node(node_element, "constraining_quadrilaterals", nodeData);
	}
	else{
	//	return an emtpy node
		return m_doc.allocate_node(node_element, "constraining_quadrilaterals");
	}
}

rapidxml::xml_node<>* GridWriterUGX::
create_constrained_quadrilateral_node(ConstrainedQuadrilateralIterator quadsBegin,
									  ConstrainedQuadrilateralIterator quadsEnd,
									  AAVrtIndex aaIndVRT,
									  AAFaceIndex aaIndFACE)
{
//	write the elements to a temporary stream
	stringstream ss;
	for(ConstrainedQuadrilateralIterator iter = quadsBegin; iter != quadsEnd; ++iter)
	{
		ss << aaIndVRT[(*iter)->vertex(0)] << " " << aaIndVRT[(*iter)->vertex(1)] << " "
			<< aaIndVRT[(*iter)->vertex(2)] << " " << aaIndVRT[(*iter)->vertex(3)] << " " ;
	//	write index of associated constraining element
	//	codes:	-1: no constraining element
	//			0: vertex. index follows
	//			1: edge. index follows
	//			2: face. index follows
	//			3: volume. index follows
		Face* cf = dynamic_cast<Face*>((*iter)->get_constraining_object());
		if(cf)
			ss << "2 " << aaIndFACE[cf] << " ";
		else
			ss << "-1 ";
	}

	if(ss.str().size() > 0){
	//	allocate a string and erase last character(' ')
		char* nodeData = m_doc.allocate_string(ss.str().c_str(), ss.str().size());
		nodeData[ss.str().size()-1] = 0;
	//	create and return the node
		return m_doc.allocate_node(node_element, "constrained_quadrilaterals", nodeData);
	}
	else{
	//	return an emtpy node
		return m_doc.allocate_node(node_element, "constrained_quadrilaterals");
	}
}

rapidxml::xml_node<>* GridWriterUGX::
create_tetrahedron_node(TetrahedronIterator tetsBegin,
						  TetrahedronIterator tetsEnd,
						  AAVrtIndex aaIndVRT)
{
//	write the elements to a temporary stream
	stringstream ss;
	for(TetrahedronIterator iter = tetsBegin; iter != tetsEnd; ++iter)
	{
		ss << aaIndVRT[(*iter)->vertex(0)] << " " << aaIndVRT[(*iter)->vertex(1)] << " "
			<< aaIndVRT[(*iter)->vertex(2)] << " " << aaIndVRT[(*iter)->vertex(3)] << " " ;
	}

	if(ss.str().size() > 0){
	//	allocate a string and erase last character(' ')
		char* nodeData = m_doc.allocate_string(ss.str().c_str(), ss.str().size());
		nodeData[ss.str().size()-1] = 0;
	//	create and return the node
		return m_doc.allocate_node(node_element, "tetrahedrons", nodeData);
	}
	else{
	//	return an emtpy node
		return m_doc.allocate_node(node_element, "tetrahedrons");
	}
}

rapidxml::xml_node<>* GridWriterUGX::
create_hexahedron_node(HexahedronIterator hexasBegin,
						  HexahedronIterator hexasEnd,
						  AAVrtIndex aaIndVRT)
{
//	write the elements to a temporary stream
	stringstream ss;
	for(HexahedronIterator iter = hexasBegin; iter != hexasEnd; ++iter)
	{
		ss << aaIndVRT[(*iter)->vertex(0)] << " " << aaIndVRT[(*iter)->vertex(1)] << " "
			<< aaIndVRT[(*iter)->vertex(2)] << " " << aaIndVRT[(*iter)->vertex(3)] << " "
			<< aaIndVRT[(*iter)->vertex(4)] << " " << aaIndVRT[(*iter)->vertex(5)] << " "
			<< aaIndVRT[(*iter)->vertex(6)] << " " << aaIndVRT[(*iter)->vertex(7)] << " ";
	}

	if(ss.str().size() > 0){
	//	allocate a string and erase last character(' ')
		char* nodeData = m_doc.allocate_string(ss.str().c_str(), ss.str().size());
		nodeData[ss.str().size()-1] = 0;
	//	create and return the node
		return m_doc.allocate_node(node_element, "hexahedrons", nodeData);
	}
	else{
	//	return an emtpy node
		return m_doc.allocate_node(node_element, "hexahedrons");
	}
}

rapidxml::xml_node<>* GridWriterUGX::
create_prism_node(PrismIterator prismsBegin,
					PrismIterator prismsEnd,
					AAVrtIndex aaIndVRT)
{
//	write the elements to a temporary stream
	stringstream ss;
	for(PrismIterator iter = prismsBegin; iter != prismsEnd; ++iter)
	{
		ss << aaIndVRT[(*iter)->vertex(0)] << " " << aaIndVRT[(*iter)->vertex(1)] << " "
			<< aaIndVRT[(*iter)->vertex(2)] << " " << aaIndVRT[(*iter)->vertex(3)] << " "
			<< aaIndVRT[(*iter)->vertex(4)] << " " << aaIndVRT[(*iter)->vertex(5)] << " ";
	}

	if(ss.str().size() > 0){
	//	allocate a string and erase last character(' ')
		char* nodeData = m_doc.allocate_string(ss.str().c_str(), ss.str().size());
		nodeData[ss.str().size()-1] = 0;
	//	create and return the node
		return m_doc.allocate_node(node_element, "prisms", nodeData);
	}
	else{
	//	return an emtpy node
		return m_doc.allocate_node(node_element, "prisms");
	}
}

rapidxml::xml_node<>* GridWriterUGX::
create_pyramid_node(PyramidIterator pyrasBegin,
					PyramidIterator pyrasEnd,
					AAVrtIndex aaIndVRT)
{
//	write the elements to a temporary stream
	stringstream ss;
	for(PyramidIterator iter = pyrasBegin; iter != pyrasEnd; ++iter)
	{
		ss << aaIndVRT[(*iter)->vertex(0)] << " " << aaIndVRT[(*iter)->vertex(1)] << " "
			<< aaIndVRT[(*iter)->vertex(2)] << " " << aaIndVRT[(*iter)->vertex(3)] << " "
			<< aaIndVRT[(*iter)->vertex(4)] << " ";
	}

	if(ss.str().size() > 0){
	//	allocate a string and erase last character(' ')
		char* nodeData = m_doc.allocate_string(ss.str().c_str(), ss.str().size());
		nodeData[ss.str().size()-1] = 0;
	//	create and return the node
		return m_doc.allocate_node(node_element, "pyramids", nodeData);
	}
	else{
	//	return an emtpy node
		return m_doc.allocate_node(node_element, "pyramids");
	}
}


////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//	implementation of GridReaderUGX
GridReaderUGX::GridReaderUGX()
{
}

GridReaderUGX::~GridReaderUGX()
{
}

const char* GridReaderUGX::
get_grid_name(size_t index) const
{
	assert(index < num_grids() && "Bad index!");
	xml_attribute<>* attrib = m_entries[index].node->first_attribute("name");
	if(attrib)
		return attrib->value();
	return NULL;
}

size_t GridReaderUGX::num_subset_handlers(size_t refGridIndex) const
{
//	access the referred grid-entry
	if(refGridIndex >= m_entries.size()){
		UG_LOG("GridReaderUGX::num_subset_handlers: bad refGridIndex. Aborting.\n");
		return 0;
	}

	return m_entries[refGridIndex].subsetHandlerEntries.size();
}

const char* GridReaderUGX::
get_subset_handler_name(size_t refGridIndex, size_t subsetHandlerIndex) const
{
	assert(refGridIndex < num_grids() && "Bad refGridIndex!");
	const GridEntry& ge = m_entries[refGridIndex];
	assert(subsetHandlerIndex < ge.subsetHandlerEntries.size() && "Bad subsetHandlerIndex!");

	xml_attribute<>* attrib = ge.subsetHandlerEntries[subsetHandlerIndex].node->first_attribute("name");
	if(attrib)
		return attrib->value();
	return NULL;
}

bool GridReaderUGX::
subset_handler(ISubsetHandler& shOut,
					size_t subsetHandlerIndex,
					size_t refGridIndex)
{
//	access the referred grid-entry
	if(refGridIndex >= m_entries.size()){
		UG_LOG("GridReaderUGX::subset_handler: bad refGridIndex. Aborting.\n");
		return false;
	}

	GridEntry& gridEntry = m_entries[refGridIndex];

//	get the referenced subset-handler entry
	if(subsetHandlerIndex >= gridEntry.subsetHandlerEntries.size()){
		UG_LOG("GridReaderUGX::subset_handler: bad subsetHandlerIndex. Aborting.\n");
		return false;
	}

	SubsetHandlerEntry& shEntry = gridEntry.subsetHandlerEntries[subsetHandlerIndex];
	shEntry.sh = &shOut;

	xml_node<>* subsetNode = shEntry.node->first_node("subset");
	size_t subsetInd = 0;
	while(subsetNode)
	{
	//	set subset info
	//	retrieve an initial subset-info from shOut, so that initialised values are kept.
		SubsetInfo si = shOut.subset_info(subsetInd);

		xml_attribute<>* attrib = subsetNode->first_attribute("name");
		if(attrib)
			si.name = attrib->value();

		attrib = subsetNode->first_attribute("color");
		if(attrib){
			stringstream ss(attrib->value(), ios_base::in);
			for(size_t i = 0; i < 4; ++i)
				ss >> si.color[i];
		}

		attrib = subsetNode->first_attribute("state");
		if(attrib){
			stringstream ss(attrib->value(), ios_base::in);
			size_t state;
			ss >> state;
			si.subsetState = (uint)state;
		}

		shOut.set_subset_info(subsetInd, si);

	//	read elements of this subset
		if(shOut.elements_are_supported(SHE_VERTEX))
			read_subset_handler_elements<VertexBase>(shOut, "vertices",
													 subsetNode, subsetInd,
													 gridEntry.vertices);
		if(shOut.elements_are_supported(SHE_EDGE))
			read_subset_handler_elements<EdgeBase>(shOut, "edges",
													 subsetNode, subsetInd,
													 gridEntry.edges);
		if(shOut.elements_are_supported(SHE_FACE))
			read_subset_handler_elements<Face>(shOut, "faces",
												 subsetNode, subsetInd,
												 gridEntry.faces);
		if(shOut.elements_are_supported(SHE_VOLUME))
			read_subset_handler_elements<Volume>(shOut, "volumes",
												 subsetNode, subsetInd,
												 gridEntry.volumes);
	//	next subset
		subsetNode = subsetNode->next_sibling("subset");
		++subsetInd;
	}

	return true;
}

template <class TGeomObj>
bool GridReaderUGX::
read_subset_handler_elements(ISubsetHandler& shOut,
							 const char* elemNodeName,
							 rapidxml::xml_node<>* subsetNode,
							 int subsetIndex,
							 std::vector<TGeomObj*>& vElems)
{
	xml_node<>* elemNode = subsetNode->first_node(elemNodeName);

	while(elemNode)
	{
	//	read the indices
		stringstream ss(elemNode->value(), ios_base::in);

		size_t index;
		while(!ss.eof()){
			ss >> index;
			if(ss.fail())
				continue;

			if(index < vElems.size()){
				shOut.assign_subset(vElems[index], subsetIndex);
			}
			else{
				UG_LOG("Bad element index in subset-node " << elemNodeName <<
						": " << index << ". Ignoring element.\n");
				return false;
			}
		}

	//	get next element node
		elemNode = elemNode->next_sibling(elemNodeName);
	}

	return true;
}


///	returns the number of selectors for the given grid
size_t GridReaderUGX::
num_selectors(size_t refGridIndex) const
{
//	access the referred grid-entry
	if(refGridIndex >= m_entries.size()){
		UG_LOG("GridReaderUGX::num_selectors: bad refGridIndex. Aborting.\n");
		return 0;
	}

	return m_entries[refGridIndex].selectorEntries.size();
}

///	returns the name of the given selector
const char* GridReaderUGX::
get_selector_name(size_t refGridIndex, size_t selectorIndex) const
{
	assert(refGridIndex < num_grids() && "Bad refGridIndex!");
	const GridEntry& ge = m_entries[refGridIndex];
	assert(selectorIndex < ge.selectorEntries.size() && "Bad selectorIndex!");

	xml_attribute<>* attrib = ge.selectorEntries[selectorIndex].node->first_attribute("name");
	if(attrib)
		return attrib->value();
	return NULL;
}

///	fills the given selector
bool GridReaderUGX::
selector(ISelector& selOut, size_t selectorIndex, size_t refGridIndex)
{
//	access the referred grid-entry
	if(refGridIndex >= m_entries.size()){
		UG_LOG("GridReaderUGX::selector: bad refGridIndex. Aborting.\n");
		return false;
	}

	GridEntry& gridEntry = m_entries[refGridIndex];

//	get the referenced subset-handler entry
	if(selectorIndex >= gridEntry.selectorEntries.size()){
		UG_LOG("GridReaderUGX::selector: bad selectorIndex. Aborting.\n");
		return false;
	}

	SelectorEntry& selEntry = gridEntry.selectorEntries[selectorIndex];
	selEntry.sel = &selOut;

	xml_node<>* selectorNode = selEntry.node;

//	read elements of this subset
	if(selOut.elements_are_supported(SHE_VERTEX))
		read_selector_elements<VertexBase>(selOut, "vertices",
											selectorNode,
											gridEntry.vertices);
	if(selOut.elements_are_supported(SHE_EDGE))
		read_selector_elements<EdgeBase>(selOut, "edges",
											selectorNode,
											gridEntry.edges);
	if(selOut.elements_are_supported(SHE_FACE))
		read_selector_elements<Face>(selOut, "faces",
										selectorNode,
										gridEntry.faces);
	if(selOut.elements_are_supported(SHE_VOLUME))
		read_selector_elements<Volume>(selOut, "volumes",
										selectorNode,
										gridEntry.volumes);

	return true;
}

template <class TGeomObj>
bool GridReaderUGX::
read_selector_elements(ISelector& selOut, const char* elemNodeName,
				   	   rapidxml::xml_node<>* selNode,
				   	   std::vector<TGeomObj*>& vElems)
{
	xml_node<>* elemNode = selNode->first_node(elemNodeName);

	while(elemNode)
	{
	//	read the indices
		stringstream ss(elemNode->value(), ios_base::in);

		size_t index;
		int state;

		while(!ss.eof()){
			ss >> index;
			if(ss.fail())
				continue;

			ss >> state;
			if(ss.fail())
				continue;

			if(index < vElems.size()){
				selOut.select(vElems[index], state);
			}
			else{
				UG_LOG("Bad element index in subset-node " << elemNodeName <<
						": " << index << ". Ignoring element.\n");
				return false;
			}
		}

	//	get next element node
		elemNode = elemNode->next_sibling(elemNodeName);
	}
	return true;
}


bool GridReaderUGX::
parse_file(const char* filename)
{
	ifstream in(filename, ios::binary);
	if(!in)
		return false;

//	get the length of the file
	streampos posStart = in.tellg();
	in.seekg(0, ios_base::end);
	streampos posEnd = in.tellg();
	streamsize size = posEnd - posStart;

//	go back to the start of the file
	in.seekg(posStart);

//	read the whole file en-block and terminate it with 0
	char* fileContent = m_doc.allocate_string(0, size + 1);
	in.read(fileContent, size);
	fileContent[size] = 0;
	in.close();

//	parse the xml-data
	m_doc.parse<0>(fileContent);

//	notify derived classes that a new document has been parsed.
	return new_document_parsed();
}

bool GridReaderUGX::
new_document_parsed()
{
//	update entries
	m_entries.clear();

//	iterate through all grids
	xml_node<>* curNode = m_doc.first_node("grid");
	while(curNode){
		m_entries.push_back(GridEntry(curNode));
		GridEntry& gridEntry = m_entries.back();

	//	collect associated subset handlers
		xml_node<>* curSHNode = curNode->first_node("subset_handler");
		while(curSHNode){
			gridEntry.subsetHandlerEntries.push_back(SubsetHandlerEntry(curSHNode));
			curSHNode = curSHNode->next_sibling("subset_handler");
		}

	//	collect associated selectors
		xml_node<>* curSelNode = curNode->first_node("selector");
		while(curSelNode){
			gridEntry.selectorEntries.push_back(SelectorEntry(curSelNode));
			curSelNode = curSelNode->next_sibling("selector");
		}

		curNode = curNode->next_sibling("grid");
	}

	return true;
}

bool GridReaderUGX::
create_edges(std::vector<EdgeBase*>& edgesOut,
			Grid& grid, rapidxml::xml_node<>* node,
			std::vector<VertexBase*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the edges
	int i1, i2;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2;

	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_edges: invalid vertex index: "
					"(" << i1 << ", " << i2 << ")\n");
			return false;
		}

	//	create the edge
		edgesOut.push_back(*grid.create<Edge>(EdgeDescriptor(vrts[i1], vrts[i2])));
	}

	return true;
}

bool GridReaderUGX::
create_constraining_edges(std::vector<EdgeBase*>& edgesOut,
						  Grid& grid, rapidxml::xml_node<>* node,
			 			  std::vector<VertexBase*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the edges
	int i1, i2;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2;

	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_constraining_edges: invalid vertex index.\n");
			return false;
		}

	//	create the edge
		edgesOut.push_back(*grid.create<ConstrainingEdge>(EdgeDescriptor(vrts[i1], vrts[i2])));
	}

	return true;
}

bool GridReaderUGX::
create_constrained_edges(std::vector<EdgeBase*>& edgesOut,
						  std::vector<std::pair<int, int> >& constrainingObjsOut,
						  Grid& grid, rapidxml::xml_node<>* node,
			 			  std::vector<VertexBase*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the edges
	int i1, i2;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2;

	//	read the type and index of the constraining object
		int conObjType, conObjIndex;
		ss >> conObjType;

		if(conObjType != -1)
			ss >> conObjIndex;

	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_edges: invalid vertex index.\n");
			return false;
		}

	//	create the edge
		ConstrainedEdge* edge = *grid.create<ConstrainedEdge>(EdgeDescriptor(vrts[i1], vrts[i2]));
		edgesOut.push_back(edge);

	//	add conObjType and conObjIndex to their list
		constrainingObjsOut.push_back(std::make_pair(conObjType, conObjIndex));
	}

	return true;
}

bool GridReaderUGX::
create_triangles(std::vector<Face*>& facesOut,
				  Grid& grid, rapidxml::xml_node<>* node,
				  std::vector<VertexBase*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the triangles
	int i1, i2, i3;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2 >> i3;

	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd ||
		   i3 < 0 || i3 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_triangles: invalid vertex index.\n");
			return false;
		}

	//	create the triangle
		facesOut.push_back(
			*grid.create<Triangle>(TriangleDescriptor(vrts[i1], vrts[i2], vrts[i3])));
	}

	return true;
}

bool GridReaderUGX::
create_constraining_triangles(std::vector<Face*>& facesOut,
					  Grid& grid, rapidxml::xml_node<>* node,
					  std::vector<VertexBase*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the triangles
	int i1, i2, i3;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2 >> i3;

	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd ||
		   i3 < 0 || i3 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_constraining_triangles: invalid vertex index.\n");
			return false;
		}

	//	create the triangle
		facesOut.push_back(
			*grid.create<ConstrainingTriangle>(TriangleDescriptor(vrts[i1], vrts[i2], vrts[i3])));
	}

	return true;
}

bool GridReaderUGX::
create_constrained_triangles(std::vector<Face*>& facesOut,
					  std::vector<std::pair<int, int> >& constrainingObjsOut,
					  Grid& grid, rapidxml::xml_node<>* node,
					  std::vector<VertexBase*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the triangles
	int i1, i2, i3;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2 >> i3;

	//	read the type and index of the constraining object
		int conObjType, conObjIndex;
		ss >> conObjType;

		if(conObjType != -1)
			ss >> conObjIndex;
			
	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd ||
		   i3 < 0 || i3 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_constraining_triangles: invalid vertex index.\n");
			return false;
		}

	//	create the triangle
		facesOut.push_back(
			*grid.create<ConstrainedTriangle>(TriangleDescriptor(vrts[i1], vrts[i2], vrts[i3])));
			
	//	add conObjType and conObjIndex to their list
		constrainingObjsOut.push_back(std::make_pair(conObjType, conObjIndex));
	}

	return true;
}

bool GridReaderUGX::
create_quadrilaterals(std::vector<Face*>& facesOut,
					   Grid& grid, rapidxml::xml_node<>* node,
					   std::vector<VertexBase*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the quadrilaterals
	int i1, i2, i3, i4;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2 >> i3 >> i4;

	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd ||
		   i3 < 0 || i3 > maxInd ||
		   i4 < 0 || i4 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_quadrilaterals: invalid vertex index.\n");
			return false;
		}

	//	create the quad
		facesOut.push_back(
			*grid.create<Quadrilateral>(QuadrilateralDescriptor(vrts[i1], vrts[i2],
															   vrts[i3], vrts[i4])));
	}

	return true;
}

bool GridReaderUGX::
create_constraining_quadrilaterals(std::vector<Face*>& facesOut,
					  Grid& grid, rapidxml::xml_node<>* node,
					  std::vector<VertexBase*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the quadrilaterals
	int i1, i2, i3, i4;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2 >> i3 >> i4;

	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd ||
		   i3 < 0 || i3 > maxInd ||
		   i4 < 0 || i4 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_quadrilaterals: invalid vertex index.\n");
			return false;
		}

	//	create the quad
		facesOut.push_back(
			*grid.create<ConstrainingQuadrilateral>(QuadrilateralDescriptor(
															vrts[i1], vrts[i2],
															vrts[i3], vrts[i4])));
	}

	return true;
}

bool GridReaderUGX::
create_constrained_quadrilaterals(std::vector<Face*>& facesOut,
					  std::vector<std::pair<int, int> >& constrainingObjsOut,
					  Grid& grid, rapidxml::xml_node<>* node,
					  std::vector<VertexBase*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the quadrilaterals
	int i1, i2, i3, i4;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2 >> i3 >> i4;

	//	read the type and index of the constraining object
		int conObjType, conObjIndex;
		ss >> conObjType;

		if(conObjType != -1)
			ss >> conObjIndex;
			
	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd ||
		   i3 < 0 || i3 > maxInd ||
		   i4 < 0 || i4 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_quadrilaterals: invalid vertex index.\n");
			return false;
		}

	//	create the quad
		facesOut.push_back(
			*grid.create<ConstrainedQuadrilateral>(QuadrilateralDescriptor(
															vrts[i1], vrts[i2],
															vrts[i3], vrts[i4])));
	
	//	add conObjType and conObjIndex to their list
		constrainingObjsOut.push_back(std::make_pair(conObjType, conObjIndex));
	}

	return true;
}

					  
bool GridReaderUGX::
create_tetrahedrons(std::vector<Volume*>& volsOut,
					 Grid& grid, rapidxml::xml_node<>* node,
					 std::vector<VertexBase*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the tetrahedrons
	int i1, i2, i3, i4;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2 >> i3 >> i4;

	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd ||
		   i3 < 0 || i3 > maxInd ||
		   i4 < 0 || i4 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_tetrahedrons: invalid vertex index.\n");
			return false;
		}

	//	create the element
		volsOut.push_back(
			*grid.create<Tetrahedron>(TetrahedronDescriptor(vrts[i1], vrts[i2],
														   vrts[i3], vrts[i4])));
	}

	return true;
}

bool GridReaderUGX::
create_hexahedrons(std::vector<Volume*>& volsOut,
					Grid& grid, rapidxml::xml_node<>* node,
					std::vector<VertexBase*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the hexahedrons
	int i1, i2, i3, i4, i5, i6, i7, i8;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2 >> i3 >> i4 >> i5 >> i6 >> i7 >> i8;

	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd ||
		   i3 < 0 || i3 > maxInd ||
		   i4 < 0 || i4 > maxInd ||
		   i5 < 0 || i5 > maxInd ||
		   i6 < 0 || i6 > maxInd ||
		   i7 < 0 || i7 > maxInd ||
		   i8 < 0 || i8 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_hexahedrons: invalid vertex index.\n");
			return false;
		}

	//	create the element
		volsOut.push_back(
			*grid.create<Hexahedron>(HexahedronDescriptor(vrts[i1], vrts[i2], vrts[i3], vrts[i4],
														  vrts[i5], vrts[i6], vrts[i7], vrts[i8])));
	}

	return true;
}

bool GridReaderUGX::
create_prisms(std::vector<Volume*>& volsOut,
			  Grid& grid, rapidxml::xml_node<>* node,
			  std::vector<VertexBase*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the hexahedrons
	int i1, i2, i3, i4, i5, i6;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2 >> i3 >> i4 >> i5 >> i6;

	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd ||
		   i3 < 0 || i3 > maxInd ||
		   i4 < 0 || i4 > maxInd ||
		   i5 < 0 || i5 > maxInd ||
		   i6 < 0 || i6 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_prisms: invalid vertex index.\n");
			return false;
		}

	//	create the element
		volsOut.push_back(
			*grid.create<Prism>(PrismDescriptor(vrts[i1], vrts[i2], vrts[i3], vrts[i4],
												vrts[i5], vrts[i6])));
	}

	return true;
}

bool GridReaderUGX::
create_pyramids(std::vector<Volume*>& volsOut,
				Grid& grid, rapidxml::xml_node<>* node,
				std::vector<VertexBase*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the hexahedrons
	int i1, i2, i3, i4, i5;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2 >> i3 >> i4 >> i5;

	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd ||
		   i3 < 0 || i3 > maxInd ||
		   i4 < 0 || i4 > maxInd ||
		   i5 < 0 || i5 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_pyramids: invalid vertex index.\n");
			return false;
		}

	//	create the element
		volsOut.push_back(
			*grid.create<Pyramid>(PyramidDescriptor(vrts[i1], vrts[i2], vrts[i3],
													vrts[i4], vrts[i5])));
	}

	return true;
}



UGXFileInfo::UGXFileInfo() :
	m_fileParsed(false)
{
}

bool UGXFileInfo::parse_file(const char* filename)
{
	ifstream in(filename, ios::binary);
	if(!in)
		return false;

//	get the length of the file
	streampos posStart = in.tellg();
	in.seekg(0, ios_base::end);
	streampos posEnd = in.tellg();
	streamsize size = posEnd - posStart;

//	go back to the start of the file
	in.seekg(posStart);

//	read the whole file en-block and terminate it with 0
	rapidxml::xml_document<> doc;
	char* fileContent = doc.allocate_string(0, size + 1);
	in.read(fileContent, size);
	fileContent[size] = 0;
	in.close();

//	parse the xml-data
	doc.parse<0>(fileContent);

	xml_node<>* curNode = doc.first_node("grid");
	while(curNode){
		m_grids.push_back(GridInfo());
		GridInfo& gInfo = m_grids.back();
		gInfo.m_name = node_name(curNode);

	//	collect associated subset handlers
		xml_node<>* curSHNode = curNode->first_node("subset_handler");
		while(curSHNode){
			gInfo.m_subsetHandlers.push_back(SubsetHandlerInfo());
			SubsetHandlerInfo& shInfo = gInfo.m_subsetHandlers.back();
			shInfo.m_name = node_name(curSHNode);

			xml_node<>* curSubsetNode = curSHNode->first_node("subset");

			while(curSubsetNode){
				shInfo.m_subsets.push_back(SubsetInfo());
				SubsetInfo& sInfo = shInfo.m_subsets.back();
				sInfo.m_name = node_name(curSubsetNode);
				curSubsetNode = curSubsetNode->next_sibling("subset");
			}

			curSHNode = curSHNode->next_sibling("subset_handler");
		}

	//	fill m_hasVertices, ...
		gInfo.m_hasVertices = curNode->first_node("vertices") != NULL;
		gInfo.m_hasVertices |= curNode->first_node("constrained_vertices") != NULL;

		gInfo.m_hasEdges = curNode->first_node("edges") != NULL;
		gInfo.m_hasEdges |= curNode->first_node("constraining_edges") != NULL;
		gInfo.m_hasEdges |= curNode->first_node("constrained_edges") != NULL;

		gInfo.m_hasFaces = curNode->first_node("triangles") != NULL;
		gInfo.m_hasFaces |= curNode->first_node("constraining_triangles") != NULL;
		gInfo.m_hasFaces |= curNode->first_node("constrained_triangles") != NULL;
		gInfo.m_hasFaces |= curNode->first_node("quadrilaterals") != NULL;
		gInfo.m_hasFaces |= curNode->first_node("constraining_quadrilaterals") != NULL;
		gInfo.m_hasFaces |= curNode->first_node("constrained_quadrilaterals") != NULL;

		gInfo.m_hasVolumes = curNode->first_node("tetrahedrons") != NULL;
		gInfo.m_hasVolumes |= curNode->first_node("hexahedrons") != NULL;
		gInfo.m_hasVolumes |= curNode->first_node("prisms") != NULL;
		gInfo.m_hasVolumes |= curNode->first_node("pyramids") != NULL;

		curNode = curNode->next_sibling("grid");
	}

	m_fileParsed = true;
	return true;
}

size_t UGXFileInfo::num_grids() const
{
	check_file_parsed();
	return m_grids.size();
}

size_t UGXFileInfo::num_subset_handlers(size_t gridInd) const
{
	return grid_info(gridInd).m_subsetHandlers.size();
}

size_t UGXFileInfo::num_subsets(size_t gridInd, size_t shInd) const
{
	return subset_handler_info(gridInd, shInd).m_subsets.size();
}

std::string UGXFileInfo::grid_name(size_t gridInd) const
{
	return grid_info(gridInd).m_name;
}

std::string UGXFileInfo::subset_handler_name(size_t gridInd, size_t shInd) const
{
	return subset_handler_info(gridInd, shInd).m_name;
}

std::string UGXFileInfo::subset_name(size_t gridInd, size_t shInd, size_t subsetInd) const
{
	return subset_info(gridInd, shInd, subsetInd).m_name;
}

bool UGXFileInfo::grid_has_vertices(size_t gridInd) const
{
	return grid_info(gridInd).m_hasVertices;
}

bool UGXFileInfo::grid_has_edges(size_t gridInd) const
{
	return grid_info(gridInd).m_hasEdges;
}

bool UGXFileInfo::grid_has_faces(size_t gridInd) const
{
	return grid_info(gridInd).m_hasFaces;
}

bool UGXFileInfo::grid_has_volumes(size_t gridInd) const
{
	return grid_info(gridInd).m_hasVolumes;
}

int UGXFileInfo::grid_world_dimension(size_t gridInd) const
{
	const GridInfo& gi = grid_info(gridInd);

	if(gi.m_hasVolumes)
		return 3;
	if(gi.m_hasFaces)
		return 2;
	if(gi.m_hasEdges)
		return 1;

	return 0;
}



std::string UGXFileInfo::node_name(rapidxml::xml_node<>* n) const
{
	xml_attribute<>* attrib = n->first_attribute("name");
	if(attrib)
		return attrib->value();
	return "";
}

void UGXFileInfo::check_file_parsed() const
{
	if(!m_fileParsed){
		UG_THROW("UGXFileInfo: no file has been parsed!");
	}
}

const UGXFileInfo::GridInfo&
UGXFileInfo::grid_info(size_t index) const
{
	check_file_parsed();
	if(index >= m_grids.size()){
		UG_THROW("Grid index out of range: " << index
				 << ". Num grids available: " << m_grids.size());
	}

	return m_grids[index];
}

const UGXFileInfo::SubsetHandlerInfo&
UGXFileInfo::subset_handler_info(size_t gridInd, size_t shInd) const
{
	const GridInfo& gi = grid_info(gridInd);
	if(shInd >= gi.m_subsetHandlers.size()){
		UG_THROW("SubsetHandler index out of range: " << shInd
				 << ". Num subset-handlers available: "
				 << gi.m_subsetHandlers.size());
	}

	return gi.m_subsetHandlers[shInd];
}

const UGXFileInfo::SubsetInfo&
UGXFileInfo::subset_info(size_t gridInd, size_t shInd, size_t subsetInd) const
{
	const SubsetHandlerInfo& shInfo = subset_handler_info(gridInd, shInd);
	if(subsetInd >= shInfo.m_subsets.size()){
		UG_THROW("Subset index out of range: " << subsetInd
				 << ". Num subset available: "
				 << shInfo.m_subsets.size());
	}

	return shInfo.m_subsets[subsetInd];
}

}//	end of namespace

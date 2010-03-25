// created by Sebastian Reiter
// s.b.reiter@googlemail.com
// y09 m08 d17

#ifndef __H__LIB_GRID__DISTRIBUTED_GRID__
#define __H__LIB_GRID__DISTRIBUTED_GRID__

#include <map>
#include <vector>
#include "lib_grid/lg_base.h"
#include "parallel_grid_layout.h"

namespace ug
{
enum ElementStatus
{
	ES_NONE = 0,
	ES_SCHEDULED_FOR_INTERFACE = 1 << 1,
	ES_IN_INTERFACE = 1 << 2,
	ES_MASTER = 1 << 3,
	ES_SLAVE = 1 << 4
};


class DistributedGridManager : public GridObserver
{
	public:
		DistributedGridManager();
		DistributedGridManager(Grid& grid);
		virtual ~DistributedGridManager();
		
	//	assignment
		void assign(Grid& grid);
			
	//	layout access
	/**	if you change the layout externally, be sure to call
	 *	DistributedGrid::layout_changed() afterwards.*/
		inline GridLayoutMap& grid_layout_map()				{return m_gridLayoutMap;}
		inline const GridLayoutMap& grid_layout_map() const	{return m_gridLayoutMap;}	
		
	///	call this method if you altered the layout externally.
	/**	This should be done as seldom as possible.
	 *	If you only added elements you may set addedElemsOnly to true.
	 *	The complexity in this case is proportional to the number of elements
	 *	in the layout.
	 *	If you removed elements or if you are unsure what operations have been
	 *	performed on the layout, you have to set addedElemsOnly to false
	 *	(the default value). Complexity in this case is proportional to the
	 *	number of elements in the underlying grid (or numer of elements in
	 *	the layout - whichever is higher).*/
	 	void grid_layouts_changed(bool addedElemsOnly = false);
		
		
	protected:
		template <class TGeomObj>
		void reset_elem_infos();
		
		template <class TGeomObj, class TLayoutMap>
		void update_elem_info(TLayoutMap& layoutMap, int nodeType, byte newStatus);

	protected:
		template <class TGeomObj>
		class ElemInfo
		{
			public:
			//	types
				typedef typename GridLayoutMap::template Types<TGeomObj>
						::Interface		Interface;
				typedef typename Interface::iterator InterfaceElemIter;
				typedef std::pair<Interface*, InterfaceElemIter> Entry;

				typedef std::list<Entry>				EntryList;
				typedef typename EntryList::iterator	EntryIterator;
				
			//	methods
				ElemInfo()	: m_status(ES_NONE)				{}
				
				void reset()								{m_status = ES_NONE; m_entries.clear();}
				
				void add_entry(Interface* interface,
								InterfaceElemIter iter)		{m_entries.push_back(Entry(interface, iter));}
				
				void remove_entry(Interface* interface)		{m_entries.erase(find_entry(interface));}
				
				inline EntryIterator entries_begin()		{return m_entries.begin();}
				inline EntryIterator entries_end()			{return m_entries.end();}
				
				EntryIterator find_entry(Interface* interface)	{return find(entries_begin(), entries_end(), interface);}
				
				void set_status(byte status)				{m_status = status;}
				byte get_status()							{return m_status;}
				
			protected:
				EntryList	m_entries;
				byte		m_status;
		};
		
		typedef ElemInfo<VertexBase>	ElemInfoVrt;
		typedef ElemInfo<EdgeBase>	ElemInfoEdge;
		typedef ElemInfo<Face>		ElemInfoFace;
		typedef ElemInfo<Volume>		ElemInfoVol;
		
		typedef util::Attachment<ElemInfoVrt>	AElemInfoVrt;
		typedef util::Attachment<ElemInfoEdge>	AElemInfoEdge;
		typedef util::Attachment<ElemInfoFace>	AElemInfoFace;
		typedef util::Attachment<ElemInfoVol>	AElemInfoVol;
		
	protected:
		inline ElemInfoVrt& elem_info(VertexBase* ele)	{return m_aaElemInfoVRT[ele];}
		inline ElemInfoEdge& elem_info(EdgeBase* ele)	{return m_aaElemInfoEDGE[ele];}
		inline ElemInfoFace& elem_info(Face* ele)		{return m_aaElemInfoFACE[ele];}
		inline ElemInfoVol& elem_info(Volume* ele)		{return m_aaElemInfoVOL[ele];}

	protected:
		Grid*	m_pGrid;

		AElemInfoVrt	m_aElemInfoVrt;
		AElemInfoEdge	m_aElemInfoEdge;
		AElemInfoFace	m_aElemInfoFace;
		AElemInfoVol	m_aElemInfoVol;
};

#ifdef __OLD_IMPLEMENTATION__
////////////////////////////////////////////////////////////////////////
///	Helps to create new interface-elements in the correct order.
/**
 * Between calls to begin_ordered_element_insertion()
 * and end_ordered_element_insertion(),
 * instances of this class will collect all created elements that have
 * been created from parent-elements that lie on an interface.
 * On end_element_creation() those elements will be added to interfaces
 * of the associated \sa GridCommunicationSet. The order in which those
 * elements are inserted is the same as the order that their parents
 * have in their interfaces.
 */
class DistributedGrid : public GridObserver
{		
	public:
		DistributedGrid();
		DistributedGrid(Grid& grid);
		
		virtual ~DistributedGridObserver();
		
	//	assignment
		void assign(Grid& grid);
			
	//	layout access
	/**	if you change the layout externally, be sure to call
	 *	DistributedGrid::layout_changed() afterwards.*/
		inline GridLayoutMap& grid_layout_map()				{return m_gridLayoutMap;}
		inline const GridLayoutMap& grid_layout_map() const	{return m_gridLayoutMap;}	
		
	///	call this method if you altered the layout externally.
	/**	This should be done as seldom as possible.
	 *	If you only added elements you may set addedElemsOnly to true.
	 *	The complexity in this case is proportional to the number of elements
	 *	in the layout.
	 *	If you removed elements or if you are unsure what operations have been
	 *	performed on the layout, you have to set addedElemsOnly to false
	 *	(the default value). Complexity in this case is proportional to the
	 *	number of elements in the underlying grid (or numer of elements in
	 *	the layout - whichever is higher).*/
	 	void grid_layouts_changed(bool addedElemsOnly = false);
		
	//	element creation
	///	call this method before you start creating new elements in the associated grid.
	/** You shouldn't add new interfaces to the associated communication-set
	 *  between begin_ and end_element_creation.*/
		void begin_ordered_element_insertion();
		
	///	call this method when you're done with element creation.
	/**	Elements will not be added to the associated \sa GridCommunicationSet
	 *  until this method is called.*/
		void end_ordered_element_insertion();
		
	//	element-status
		inline bool check_status(VertexBase* vrt, byte status)
			{return ((get_status(vrt) & status) == status);}
		
		inline bool check_status(EdgeBase* edge, byte status)
			{return ((get_status(edge) & status) == status);}

		inline bool check_status(Face* face, byte status)
			{return ((get_status(face) & status) == status);}

		inline bool check_status(Volume* vol, byte status)
			{return ((get_status(vol) & status) == status);}
			
		inline byte get_status(VertexBase* vrt)	{return elem_info(vrt).status;}
		inline byte get_status(EdgeBase* edge)	{return elem_info(edge).status;}
		inline byte get_status(Face* face)		{return elem_info(face).status;}
		inline byte get_status(Volume* vol)		{return elem_info(vol).status;}
		
	//	grid callbacks
		virtual void registered_at_grid(Grid* grid);
		virtual void unregistered_from_grid(Grid* grid);
		virtual void elements_to_be_cleared(Grid* grid);
		
	//	vertex callbacks
		virtual void vertex_created(Grid* grid, VertexBase* vrt, GeometricObject* pParent = NULL);
		virtual void vertex_to_be_erased(Grid* grid, VertexBase* vrt);
		virtual void vertex_to_be_replaced(Grid* grid, VertexBase* vrtOld, VertexBase* vrtNew);

	//	edge callbacks
		virtual void edge_created(Grid* grid, EdgeBase* edge, GeometricObject* pParent = NULL);
		virtual void edge_to_be_erased(Grid* grid, EdgeBase* edge);
		virtual void edge_to_be_replaced(Grid* grid, EdgeBase* edgeOld, EdgeBase* edgeNew);

	//	face callbacks
		virtual void face_created(Grid* grid, Face* face, GeometricObject* pParent = NULL)				{}
		virtual void face_to_be_erased(Grid* grid, Face* face)			{}
		virtual void face_to_be_replaced(Grid* grid, Face* faceOld, Face* faceNew)	{}

	//	volume callbacks
		virtual void volume_created(Grid* grid, Volume* vol, GeometricObject* pParent = NULL)			{}
		virtual void volume_to_be_erased(Grid* grid, Volume* vol)		{}
		virtual void volume_to_be_replaced(Grid* grid, Volume* volOld, Volume* volNew)	{}
		
	protected:
		
		class ScheduledElement
		{
			public:
				ScheduledElement()	{}
				ScheduledElement(GeometricObject* obj, InterfaceNodeTypes nt,
								int procID) : pObj(obj),
											nodeType(nt),
											connectedProcID(procID)	{}
				GeometricObject*	pObj;
				InterfaceNodeTypes	nodeType;
				int					connectedProcID;
		};
		
		typedef VertexLayout::Interface::iterator	InterfaceElemIteratorVrt;
		typedef EdgeLayout::Interface::iterator		InterfaceElemIteratorEdge;
		typedef FaceLayout::Interface::iterator		InterfaceElemIteratorFace;
		typedef VolumeLayout::Interface::iterator	InterfaceElemIteratorVol;
		
		typedef std::multimap<InterfaceElemIteratorVrt,
							ScheduledElement,
							VertexLayout::Interface::cmp> 	ScheduledElemMapVrt;
							
		typedef std::multimap<InterfaceElemIteratorEdge,
							ScheduledElement,
							EdgeLayout::Interface::cmp> 	ScheduledElemMapEdge;
							
		typedef std::multimap<InterfaceElemIteratorFace,
							ScheduledElement,
							FaceLayout::Interface::cmp> 	ScheduledElemMapFace;
							
		typedef std::multimap<InterfaceElemIteratorVol,
							ScheduledElement,
							VolumeLayout::Interface::cmp> 	ScheduledElemMapVol;

/*
		typedef VertexCommunicationGroup::HNODE HVertex;
		typedef EdgeCommunicationGroup::HNODE HEdge;
		typedef FaceCommunicationGroup::HNODE HFace;
		typedef VolumeCommunicationGroup::HNODE HVolume;
*/		
		template <class TGeomObj>
		struct ElementInfo
		{
		//	types
			typedef typename GridLayoutMap::template Types<TGeomObj>
					::Interface::iterator InterfaceElemIter;
					
			struct ProcIterPair{
				ProcIterPair(int pID, InterfaceElemIter i) :
					procID(pID), iter(i)	{}
					
				int					procID;
				InterfaceElemIter	iter;
			};

			typedef std::list<ProcIterPair>	ProcIterPairList;			

		//	methods
			ElementInfo()		{}
			inline void reset()	{lstProcIterPairs.clear(); status = ES_NONE;}
			
		//	members
		///	holds pairs of (procID, elem-iterator).
			ProcIterPairList	lstProcIterPairs;
		///	the status of the element
			byte 	status;
		};
			
		typedef ElementInfo<VertexBase>	ElemInfoVrt;
		typedef ElementInfo<EdgeBase>	ElemInfoEdge;
		typedef ElementInfo<Face>		ElemInfoFace;
		typedef ElementInfo<Volume>		ElemInfoVol;
		
		typedef util::Attachment<ElemInfoVrt>	AElemInfoVrt;
		typedef util::Attachment<ElemInfoEdge>	AElemInfoEdge;
		typedef util::Attachment<ElemInfoFace>	AElemInfoFace;
		typedef util::Attachment<ElemInfoVol>	AElemInfoVol;
		
	protected:	
		void clear_scheduled_elements();
		void resize_scheduled_element_map_vecs();
		
		inline ElemInfoVrt& elem_info(VertexBase* ele)	{return m_aaElemInfoVRT[ele];}
		inline ElemInfoEdge& elem_info(EdgeBase* ele)	{return m_aaElemInfoEDGE[ele];}
		inline ElemInfoFace& elem_info(Face* ele)		{return m_aaElemInfoFACE[ele];}
		inline ElemInfoVol& elem_info(Volume* ele)		{return m_aaElemInfoVOL[ele];}
		
		byte get_geom_obj_status(GeometricObject* go);
		
		inline void set_status(VertexBase* vrt, byte status){elem_info(vrt).status = status;}
		inline void set_status(EdgeBase* edge, byte status)	{elem_info(edge).status = status;}
		inline void set_status(Face* face, byte status)		{elem_info(face).status = status;}
		inline void set_status(Volume* vol, byte status)	{elem_info(vol).status = status;}
/*
		const std::vector<int>& get_interface_indices(GeometricObject* go);
		const std::vector<int>& get_interface_entry_indices(GeometricObject* go);
*/
		template <class TGeomObj>
		void reset_elem_infos();
		
		template <class TGeomObj, class TLayoutMap>
		void update_elem_info(TLayoutMap& layoutMap, int nodeType, byte newStatus);

	///	vertex_created, edge_created, ... callbacks call this method.
		template <class TElem>
		void handle_created_element(TElem* pElem,
									GeometricObject* pParent);
		
	///	vertex_to_be_erased, edge..., ... callbacks call this method.
		template <class TElem, class TCommGrp>
		void handle_erased_element(TElem* e, TCommGrp& commGrp);
	
	///	vertex_to_be_replaced, edge..., ... callbacks call this method.	
		template <class TElem, class TCommGrp>
		void handle_replaced_element(TElem* eOld, TElem* eNew,
									TCommGrp& commGrp);
									
		template <class TScheduledElemMap>
		void perform_ordered_element_insertion(TScheduledElemMap& elemMap);
		
		template <class TElem, class TCommGrp>
		void add_element_to_interface(TElem* pElem, TCommGrp& commGrp,
										int procID, InterfaceNodeTypes nodeType);
						
	protected:
		Grid*					m_pGrid;
		GridLayoutMap			m_gridLayoutMap;
		
		ScheduledElemMapVrt		m_vrtMap;	///< holds all elements that were scheduled by vertices
		ScheduledElemMapEdge	m_edgeMap;	///< holds all elements that were scheduled by edges
		ScheduledElemMapFace	m_faceMap;	///< holds all elements that were scheduled by faces
		ScheduledElemMapVol		m_volMap;	///< holds all elements that were scheduled by volumes
		
		bool	m_bOrderedInsertionMode;
		
		AElemInfoVrt	m_aElemInfoVrt;
		AElemInfoEdge	m_aElemInfoEdge;
		AElemInfoFace	m_aElemInfoFace;
		AElemInfoVol	m_aElemInfoVol;
		
		Grid::VertexAttachmentAccessor<AElemInfoVrt>	m_aaElemInfoVRT;
		Grid::EdgeAttachmentAccessor<AElemInfoEdge>		m_aaElemInfoEDGE;
		Grid::FaceAttachmentAccessor<AElemInfoFace>		m_aaElemInfoFACE;
		Grid::VolumeAttachmentAccessor<AElemInfoVol>	m_aaElemInfoVOL;
};

#endif //__OLD_IMPLEMENTATION__
}// end of namespace

#endif

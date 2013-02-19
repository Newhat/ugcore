/*
 * approximation_space.cpp
 *
 *  Created on: 13.12.2011
 *      Author: andreasvogel
 */

#include "approximation_space.h"
#include "lib_disc/domain.h"
#include "lib_disc/common/groups_util.h"
#include "common/util/string_util.h"
#ifdef UG_PARALLEL
	#include "pcl/pcl.h"
#endif

#include "lib_disc/dof_manager/mg_dof_distribution.h"
#include "lib_disc/dof_manager/level_dof_distribution.h"
#include "lib_disc/dof_manager/surface_dof_distribution.h"
#include "grid_function.h"


namespace ug{

IApproximationSpace::
IApproximationSpace(SmartPtr<subset_handler_type> spMGSH,
                    SmartPtr<grid_type> spMG,
                    const AlgebraType& algebraType)
:	 DoFDistributionInfo(spMGSH),
     m_spMG(spMG),
 	 m_spMGSH(spMGSH),
 	 m_bAdaptionIsActive(false),
	 m_algebraType(algebraType)
#ifdef UG_PARALLEL
	, m_pDistGridMgr(NULL)
#endif
{
//	get blocksize of algebra
	const int blockSize = m_algebraType.blocksize();

//	a)	If blocksize fixed and > 1, we need grouping in dof manager. Thus,
//		the dofmanager hopefully fits (i.e. same number of grouped
//		dofs everywhere.)
	if(blockSize > 1) m_bGrouped = true;
//	b) 	If blocksize flexible, we group
	else if (blockSize == AlgebraType::VariableBlockSize) m_bGrouped = true;
//	c)	If blocksize == 1, we do not group. This will allow us to handle
//		this case for any problem.
	else if (blockSize == 1) m_bGrouped = false;
	else
		UG_THROW("Cannot determine blocksize of Algebra.");

	register_at_adaption_msg_hub();
}

IApproximationSpace::
IApproximationSpace(SmartPtr<subset_handler_type> spMGSH,
                    SmartPtr<grid_type> spMG)
	: DoFDistributionInfo(spMGSH),
	  m_spMG(spMG),
	  m_spMGSH(spMGSH),
	  m_bAdaptionIsActive(false),
	  m_algebraType(DefaultAlgebra::get())
#ifdef UG_PARALLEL
	, m_pDistGridMgr(NULL)
#endif
{
//	get blocksize of algebra
	const int blockSize = m_algebraType.blocksize();

//	a)	If blocksize fixed and > 1, we need grouping in dof manager. Thus,
//		the dofmanager hopefully fits (i.e. same number of grouped
//		dofs everywhere.)
	if(blockSize > 1) m_bGrouped = true;
//	b) 	If blocksize flexible, we group
	else if (blockSize == AlgebraType::VariableBlockSize) m_bGrouped = true;
//	c)	If blocksize == 1, we do not group. This will allow us to handle
//		this case for any problem.
	else if (blockSize == 1) m_bGrouped = false;
	else
		UG_THROW("Cannot determine blocksize of Algebra.");

	register_at_adaption_msg_hub();
}

IApproximationSpace::
~IApproximationSpace()
{
	m_vLevDD.clear();
	m_vSurfDD.clear();

	if(m_spTopSurfDD.valid())
		m_spTopSurfDD = SmartPtr<SurfaceDoFDistribution>(NULL);

	if(m_spLevMGDD.valid())
		m_spLevMGDD = SmartPtr<LevelMGDoFDistribution>(NULL);

	if(m_spSurfaceView.valid())
		m_spSurfaceView = SmartPtr<SurfaceView>(NULL);
}


void IApproximationSpace::register_at_adaption_msg_hub()
{
//	register function for grid adaption
	SPMessageHub msgHub = m_spMGSH->multi_grid()->message_hub();
	m_spGridAdaptionCallbackID =
		msgHub->register_class_callback(this,
		&ug::IApproximationSpace::grid_changed_callback);

	m_spGridCreationCallbackID =
		msgHub->register_class_callback(this,
		&ug::IApproximationSpace::grid_creation_callback);
}

void IApproximationSpace::
grid_changed_callback(const GridMessage_Adaption& msg)
{
	if(msg.adaption_begins())
		m_bAdaptionIsActive = true;

	else if(m_bAdaptionIsActive){
		if(msg.adaptive()){
			if(msg.adaption_ends())
			{
				defragment();
				m_bAdaptionIsActive = false;
			}
		}
	}

	else{
		UG_THROW("Before any grid-adaption may be performed, the approximation"
				" space has to be informed that grid-adaption shall begin. "
				"You may use IRefiner::grid_adaption_begins() or schedule "
				"an appropriate message to the associated grids message-hub.");
	}
}

void IApproximationSpace::grid_creation_callback(const GridMessage_Creation& msg)
{
	bool freeze = false;
	bool performSomething = false;

	if(msg.msg() == GMCT_CREATION_STARTS){
		freeze = true;
		performSomething = true;
	}
	else if(msg.msg() == GMCT_CREATION_STOPS){
		freeze = false;
		performSomething = true;
	//	refresh the surface view
		if(m_spSurfaceView.valid())
			m_spSurfaceView->refresh_surface_states();
	}

//	freeze / unfreeze all associated dof managers
	if(performSomething){
		if(m_spLevMGDD.valid())
			m_spLevMGDD->freeze(freeze);
		if(m_spTopSurfDD.valid())
			m_spTopSurfDD->freeze(freeze);

		for(size_t i = 0; i < m_vSurfDD.size(); ++i){
			if(m_vSurfDD[i].valid())
				m_vSurfDD[i]->freeze(freeze);
		}

	//	levelDDs are already handled since they all share m_spLevMGDD
	}
}

bool IApproximationSpace::levels_enabled() const
{
	return !m_vLevDD.empty();
}

bool IApproximationSpace::top_surface_enabled() const
{
	return m_spTopSurfDD.valid();
}

bool IApproximationSpace::surfaces_enabled() const
{
	return !m_vSurfDD.empty();
}


std::vector<ConstSmartPtr<DoFDistribution> >
IApproximationSpace::surface_dof_distributions() const
{
	std::vector<ConstSmartPtr<DoFDistribution> > vDD;
	if(num_levels() == 0) return vDD;

	const_cast<IApproximationSpace*>(this)->surf_dd_required(0,num_levels()-1);
	vDD.resize(m_vSurfDD.size());
	for(size_t i = 0; i < m_vSurfDD.size(); ++i)
		vDD[i] = m_vSurfDD[i];
	return vDD;
}

SmartPtr<DoFDistribution>
IApproximationSpace::surface_dof_distribution(int level)
{
	if(level != GridLevel::TOPLEVEL){
		surf_dd_required(level,level); return m_vSurfDD[level];
	}

	else{
		top_surf_dd_required();
		return m_spTopSurfDD;
	}
}

ConstSmartPtr<DoFDistribution>
IApproximationSpace::surface_dof_distribution(int level) const
{
	return const_cast<IApproximationSpace*>(this)->surface_dof_distribution(level);
}

std::vector<ConstSmartPtr<DoFDistribution> >
IApproximationSpace::level_dof_distributions() const
{
	std::vector<ConstSmartPtr<DoFDistribution> > vDD;
	if(num_levels() == 0) return vDD;

	const_cast<IApproximationSpace*>(this)->level_dd_required(0,num_levels()-1);
	vDD.resize(m_vLevDD.size());
	for(size_t i = 0; i < m_vLevDD.size(); ++i)
		vDD[i] = m_vLevDD[i];
	return vDD;
}

SmartPtr<DoFDistribution>
IApproximationSpace::level_dof_distribution(int level)
{
	level_dd_required(level,level); return m_vLevDD[level];
}

ConstSmartPtr<DoFDistribution>
IApproximationSpace::level_dof_distribution(int level) const
{
	const_cast<IApproximationSpace*>(this)->level_dd_required(level,level);
	return m_vLevDD[level];
}


SmartPtr<DoFDistribution>
IApproximationSpace::dof_distribution(const GridLevel& gl)
{
	if(gl.type() == GridLevel::LEVEL)
		return level_dof_distribution(gl.level());
	else if (gl.type() == GridLevel::SURFACE)
		return surface_dof_distribution(gl.level());
	else
		UG_THROW("Invalid Grid Level requested: "<<gl);
}

ConstSmartPtr<DoFDistribution>
IApproximationSpace::dof_distribution(const GridLevel& gl) const
{
	if(gl.type() == GridLevel::LEVEL)
		return level_dof_distribution(gl.level());
	else if (gl.type() == GridLevel::SURFACE)
		return surface_dof_distribution(gl.level());
	else
		UG_THROW("Invalid Grid Level requested: "<<gl);
}

void IApproximationSpace::init_levels()
{
	PROFILE_FUNC();
	if(num_levels() > 0)
		level_dd_required(0, num_levels()-1);
}

void IApproximationSpace::init_surfaces()
{
	PROFILE_FUNC();
	if(num_levels() > 0){
		surf_dd_required(0, num_levels()-1);
		top_surf_dd_required();
	}
}

void IApproximationSpace::init_top_surface()
{
	PROFILE_FUNC();
	top_surf_dd_required();
}

void IApproximationSpace::defragment()
{
	PROFILE_FUNC();
//	update surface view
	if(m_spSurfaceView.valid())
		m_spSurfaceView->refresh_surface_states();

//	defragment level dd
	if(m_spLevMGDD.valid())
		if(num_levels() > m_vLevDD.size())
			level_dd_required(m_vLevDD.size(), num_levels()-1);

	for(size_t lev = 0; lev < m_vLevDD.size(); ++lev)
		if(m_vLevDD[lev].valid()) m_vLevDD[lev]->defragment();

//	defragment top dd
	if(m_spTopSurfDD.valid()) m_spTopSurfDD->defragment();

//	defragment surface dd
	for(size_t lev = 0; lev < m_vSurfDD.size(); ++lev)
		if(m_vSurfDD[lev].valid()) m_vSurfDD[lev]->defragment();
}

void IApproximationSpace::
print_statistic(ConstSmartPtr<DoFDistribution> dd, int verboseLev) const
{
//	Total number of DoFs
	UG_LOG(std::setw(10) << ConvertNumber(dd->num_indices(),10,6) << " | ");

	const int blockSize = DefaultAlgebra::get().blocksize();

//	Overall block size
	if(blockSize != AlgebraType::VariableBlockSize) {UG_LOG(std::setw(8)  << blockSize);}
	else {UG_LOG("variable");}
	UG_LOG("  | " );

//	Subset informations
	if(verboseLev >= 1)
	{
		for(int si = 0; si < dd->num_subsets(); ++si)
		{
			UG_LOG( " (" << dd->subset_name(si) << ",");
			UG_LOG(std::setw(8) << ConvertNumber(dd->num_indices(si),8,4) << ") ");

		}
	}
}

#ifdef UG_PARALLEL
static size_t NumIndices(const IndexLayout& Layout)
{
	size_t sum = 0;
	for(IndexLayout::const_iterator iter = Layout.begin();
			iter != Layout.end(); ++iter)
		sum += Layout.interface(iter).size();
	return sum;
}
#endif

void IApproximationSpace::
print_parallel_statistic(ConstSmartPtr<DoFDistribution> dd, int verboseLev) const
{
#ifdef UG_PARALLEL
//	Get Process communicator;
	const pcl::ProcessCommunicator& pCom = dd->layouts().proc_comm();

//	hack since pcl does not support much constness
	DoFDistribution* nonconstDD = const_cast<DoFDistribution*>(dd.get());

//	compute local dof numbers of all masters; this the number of all dofs
//	minus the number of all slave dofs (double counting of slaves can not
//	occur, since each slave is only slave of one master), minus the number of
//	all vertical master dofs (double counting can occure, since a vertical master
//	can be in a horizontal slave interface.
//	Therefore: 	if there are no vert. master dofs, we can simply calculate the
//				number of DoFs by counting. If there are vert. master dofs
//				we need to remove doubles when counting.
	int numMasterDoF;
	if(NumIndices(dd->layouts().vertical_master()) > 0)
	{
	//	create vector of vertical masters and horiz. slaves, since those are
	//	not reguarded as masters on the dd. All others are masters. (Especially,
	//	also vertical slaves are masters w.r.t. computing like smoothing !!)
		std::vector<IndexLayout::Element> vIndex;
		CollectElements(vIndex, nonconstDD->layouts().vertical_master());
		CollectElements(vIndex, nonconstDD->layouts().slave(), false);

	//	sort vector and remove doubles
		std::sort(vIndex.begin(), vIndex.end());
		vIndex.erase(std::unique(vIndex.begin(), vIndex.end()),
		                         vIndex.end());

	//	now remove all horizontal masters, since they are still master though
	//	in the vert master interface
		std::vector<IndexLayout::Element> vIndex2;
		CollectElements(vIndex2, nonconstDD->layouts().master());
		for(size_t i = 0; i < vIndex2.size(); ++i)
			vIndex.erase(std::remove(vIndex.begin(), vIndex.end(), vIndex2[i]),
			             vIndex.end());

	//	the remaining DoFs are the "slaves"
		numMasterDoF = dd->num_indices() - vIndex.size();
	}
	else
	{
	//	easy case: only subtract masters from slaves
		numMasterDoF = dd->num_indices() - NumIndices(dd->layouts().slave());
	}

//	global and local values
//	we use uint64 here instead of size_t since the communication via
//	mpi does not support the usage of size_t.
	std::vector<uint64> tNumGlobal, tNumLocal;

//	write number of Masters on this process
	tNumLocal.push_back(numMasterDoF);

//	write local number of dof in a subset for all subsets. For simplicity, we
//	only communicate the total number of dofs (i.e. master+slave)
//	\todo: count slaves in subset and subtract them to get only masters
	for(int si = 0; si < dd->num_subsets(); ++si)
		tNumLocal.push_back(dd->num_indices(si));

//	resize receive array
	tNumGlobal.resize(tNumLocal.size());

//	sum up over processes
	if(!pCom.empty())
	{
		pCom.allreduce(&tNumLocal[0], &tNumGlobal[0], tNumGlobal.size(),
		               PCL_DT_UNSIGNED_LONG_LONG, PCL_RO_SUM);
	}
	else if (pcl::GetNumProcesses() == 1)
	{
		for(size_t i = 0; i < tNumGlobal.size(); ++i)
		{
			tNumGlobal[i] = tNumLocal[i];
		}
	}
	else
	{
		UG_LOG(" Unable to compute informations.");
		return;
	}

//	Total number of DoFs (last arg of 'ConvertNumber()' ("precision") is total
//  width - 4 (two for binary prefix, two for space and decimal point)
	UG_LOG(std::setw(10) << ConvertNumber(tNumGlobal[0],10,6) <<" | ");

//	Overall block size
	const int blockSize = DefaultAlgebra::get().blocksize();
	if(blockSize != AlgebraType::VariableBlockSize) {UG_LOG(std::setw(8)  << blockSize);}
	else {UG_LOG("variable");}
	UG_LOG("  | " );

//	Subset informations
	if(verboseLev>=1)
	{
		for(int si = 0; si < dd->num_subsets(); ++si)
		{
			UG_LOG( " (" << dd->subset_name(si) << ",");
			UG_LOG(std::setw(8) << ConvertNumber(tNumGlobal[si+1],8,4) << ") ");
		}
	}
#endif
}


void IApproximationSpace::print_statistic(int verboseLev) const
{
//	Write info
	UG_LOG("Statistic on DoF-Distribution");
#ifdef UG_PARALLEL
	UG_LOG(" on process " << pcl::GetProcRank() <<
	       " of " << pcl::GetNumProcesses() << " processes");
#endif
	UG_LOG(":\n");

//	check, what to print
	bool bPrintSurface = !m_vSurfDD.empty() || m_spTopSurfDD.valid();
	bool bPrintLevel = !m_vLevDD.empty();

//	Write header line
	if(bPrintLevel || bPrintSurface)
	{
		UG_LOG("         |   Total   | BlockSize | ");
		if(verboseLev >= 1) UG_LOG("(Subset, DoFs) ");
		UG_LOG("\n");
	}

//	Write Infos for Levels
	if(bPrintLevel)
	{
		UG_LOG("  Level  |\n");
		for(size_t l = 0; l < m_vLevDD.size(); ++l)
		{
			UG_LOG("  " << std::setw(5) << l << "  |");
			print_statistic(level_dof_distribution(l), verboseLev);
			UG_LOG(std::endl);
		}
	}

//	Write Infos for Surface Grid
	if(bPrintSurface)
	{
		UG_LOG(" Surface |\n");
		if(!m_vSurfDD.empty())
		{
			for(size_t l = 0; l < m_vSurfDD.size(); ++l)
			{
				UG_LOG("  " << std::setw(5) << l << "  |");
				print_statistic(surface_dof_distribution(l), verboseLev);
				UG_LOG(std::endl);
			}
		}

		if(m_spTopSurfDD.valid())
		{
			UG_LOG("     top |");
			print_statistic(surface_dof_distribution(GridLevel::TOPLEVEL), verboseLev);
			UG_LOG(std::endl);
		}
	}

//	if nothing printed
	if(!bPrintLevel && ! bPrintSurface)
	{
		UG_LOG(	"   No DoFs distributed yet (done automatically). \n"
				"   In order to force DoF creation use \n"
				"   ApproximationSpace::init_levels() or "
				"ApproximationSpace::init_surfaces().\n\n");
	}

#ifdef UG_PARALLEL
//	only in case of more than 1 proc
	if(pcl::GetNumProcesses() < 2) return;

//	header
	UG_LOG("Statistic on DoFDistribution on all Processes (m= master, s=slave):\n");

//	Write header line
	if(bPrintLevel || bPrintSurface)
	{
		UG_LOG("         |   Total   | BlockSize | ");
		if(verboseLev >= 1) UG_LOG("(Subset, DoFs) ");
		UG_LOG("\n");
	}

//	Write Infos for Levels
	if(bPrintLevel)
	{
		UG_LOG("  Level  |\n");
		for(size_t l = 0; l < m_vLevDD.size(); ++l)
		{
			UG_LOG("  " << std::setw(5) << l << "  |");
			print_parallel_statistic(level_dof_distribution(l), verboseLev);
			UG_LOG(std::endl);
		}
	}

//	Write Infos for Surface Grid
	if(bPrintSurface)
	{
		UG_LOG(" Surface |\n");
		if(!m_vSurfDD.empty())
		{
			for(size_t l = 0; l < m_vSurfDD.size(); ++l)
			{
				UG_LOG("  " << std::setw(5) << l << "  |");
				print_parallel_statistic(surface_dof_distribution(l), verboseLev);
				UG_LOG(std::endl);
			}
		}

		if(m_spTopSurfDD.valid())
		{
			UG_LOG("     top |");
			print_parallel_statistic(surface_dof_distribution(GridLevel::TOPLEVEL), verboseLev);
			UG_LOG(std::endl);
		}
	}
#endif
}

#ifdef UG_PARALLEL
static void PrintLayoutStatistic(ConstSmartPtr<DoFDistribution> dd)
{
	UG_LOG(std::setw(8) << NumIndices(dd->layouts().master()) <<" | ");
	UG_LOG(std::setw(8) << NumIndices(dd->layouts().slave()) <<" | ");
	UG_LOG(std::setw(12) << NumIndices(dd->layouts().vertical_master()) <<" | ");
	UG_LOG(std::setw(12) << NumIndices(dd->layouts().vertical_slave()));
}
#endif

void IApproximationSpace::print_layout_statistic(int verboseLev) const
{
#ifdef UG_PARALLEL
//	Write info
	UG_LOG("Layouts on Process " <<  GetLogAssistant().get_output_process() << ":\n");

//	Write header line
	UG_LOG(" Level |  Master  |  Slave   | vert. Master | vert. Slave\n");
	UG_LOG("----------------------------------------------------------\n");

//	Write Infos for Levels
	for(size_t l = 0; l < m_vLevDD.size(); ++l)
	{
		UG_LOG(" " << std::setw(5)<< l << " | ");
		PrintLayoutStatistic(level_dof_distribution(l));
		UG_LOG(std::endl);
	}

//	Write Infos for Surface Grid
	if(!m_vSurfDD.empty())
	{
		UG_LOG(" Surf  |\n");
		for(size_t l = 0; l < m_vLevDD.size(); ++l)
		{
			UG_LOG(" " << std::setw(5)<< l << " | ");
			PrintLayoutStatistic(surface_dof_distribution(l));
			UG_LOG(std::endl);
		}
	}

#else
	UG_LOG(" No Layouts in sequential code.\n");
#endif
}

void IApproximationSpace::level_dd_required(size_t fromLevel, size_t toLevel)
{
//	check correct arguments
	if(fromLevel > toLevel)
		UG_THROW("fromLevel must be smaller than toLevel");

//	check level
	if(toLevel >= this->num_levels())
		UG_THROW("Required Level "<<toLevel<<", but only "<<
					   this->num_levels()<<" in the MultiGrid.");

	dof_distribution_info_required();

//	if not yet MGLevelDD allocated
	if(!m_spLevMGDD.valid()){
		m_spLevMGDD = SmartPtr<LevelMGDoFDistribution>
					(new LevelMGDoFDistribution(m_spMG, m_spMGSH, *this,
					                            m_bGrouped));
	}

//	resize level
	if(m_vLevDD.size() < toLevel+1) m_vLevDD.resize(toLevel+1, NULL);

	surface_view_required();

//	allocate Level DD if needed
	for(size_t lvl = fromLevel; lvl <= toLevel; ++lvl)
	{
		if(!m_vLevDD[lvl].valid()){
			m_vLevDD[lvl] = SmartPtr<LevelDoFDistribution>
							(new LevelDoFDistribution(m_spLevMGDD, m_spSurfaceView, lvl));
		}
	}
}

void IApproximationSpace::surf_dd_required(size_t fromLevel, size_t toLevel)
{
//	check correct arguments
	if(fromLevel > toLevel)
		UG_THROW("fromLevel must be smaller than toLevel");

//	resize level
	if(m_vSurfDD.size() < toLevel+1) m_vSurfDD.resize(toLevel+1, NULL);

	dof_distribution_info_required();
	surface_view_required();

//	allocate Level DD if needed
	for(size_t lvl = fromLevel; lvl <= toLevel; ++lvl)
	{
		if(!m_vSurfDD[lvl].valid()){
			m_vSurfDD[lvl] = SmartPtr<SurfaceDoFDistribution>
							(new SurfaceDoFDistribution(m_spMG, m_spMGSH, *this,
									m_spSurfaceView, lvl, m_bGrouped));
		}
	}
}

void IApproximationSpace::top_surf_dd_required()
{
	dof_distribution_info_required();
	surface_view_required();
						 
//	allocate Level DD if needed
	if(!m_spTopSurfDD.valid()){
		m_spTopSurfDD = SmartPtr<SurfaceDoFDistribution>
							(new SurfaceDoFDistribution(
									m_spMG, m_spMGSH, *this,
									m_spSurfaceView, GridLevel::TOPLEVEL, m_bGrouped));
	}
}

void IApproximationSpace::surface_view_required()
{
//	allocate surface view if needed
	if(!m_spSurfaceView.valid())
		m_spSurfaceView = SmartPtr<SurfaceView>(new SurfaceView(m_spMGSH));
}

void IApproximationSpace::dof_distribution_info_required()
{
	DoFDistributionInfo::init();
}


template <typename TDomain>
ApproximationSpace<TDomain>::
ApproximationSpace(SmartPtr<domain_type> domain)
	: IApproximationSpace(domain->subset_handler(), domain->grid()),
	  m_spDomain(domain)
{
	if(!m_spDomain.valid())
		UG_THROW("Domain, passed to ApproximationSpace, is invalid.");
	if(!m_spMGSH.valid())
		UG_THROW("SubsetHandler, passed to ApproximationSpace, is invalid.");

#ifdef UG_PARALLEL
	this->set_dist_grid_mgr(domain->distributed_grid_manager());
#endif
};

template <typename TDomain>
ApproximationSpace<TDomain>::
ApproximationSpace(SmartPtr<domain_type> domain, const AlgebraType& algebraType)
	: IApproximationSpace(domain->subset_handler(), domain->grid(), algebraType),
	  m_spDomain(domain)
{
	if(!m_spDomain.valid())
		UG_THROW("Domain, passed to ApproximationSpace, is invalid.");
	if(!m_spMGSH.valid())
		UG_THROW("SubsetHandler, passed to ApproximationSpace, is invalid.");

#ifdef UG_PARALLEL
	this->set_dist_grid_mgr(domain->distributed_grid_manager());
#endif
};

} // end namespace ug

template class ug::ApproximationSpace<ug::Domain1d>;
template class ug::ApproximationSpace<ug::Domain2d>;
template class ug::ApproximationSpace<ug::Domain3d>;

//	Sebastian Reiter (sreiter), Martin Stepniewski (mstepnie)
//	s.b.reiter@googlemail.com, mastep@gmx.de
//	y09 m11 d11

#include "volume_util.h"

using namespace std;

namespace ug
{

////////////////////////////////////////////////////////////////////////
//	GetNeighbours - sreiter
void GetNeighbours(std::vector<Volume*>& vVolsOut, Grid& grid, Volume* v,
					int side, bool clearContainer)
{
	if(clearContainer)
		vVolsOut.clear();

//	if VOLOPT_AUTOGENERATE_FACES and FACEOPT_STORE_ASSOCIATED_VOLUMES are
//	activated, we may use them to find the connected volume quite fast.
	if(grid.option_is_enabled(VOLOPT_AUTOGENERATE_FACES
							| FACEOPT_STORE_ASSOCIATED_VOLUMES))
	{
		Face* f = grid.get_face(v, side);
		VolumeIterator iterEnd = grid.associated_volumes_end(f);
		for(VolumeIterator iter = grid.associated_volumes_begin(f);
			iter != iterEnd; ++iter)
		{
			if(*iter != v)
				vVolsOut.push_back(*iter);
		}

		return;
	}

//	we can't assume that associated faces exist.
//	we have to find the neighbour by hand.
//	mark all vertices of the side
	grid.begin_marking();

	FaceDescriptor fd;
	v->face(side, fd);
	uint numFaceVrts = fd.num_vertices();
	for(uint i = 0; i < numFaceVrts; ++ i)
		grid.mark(fd.vertex(i));

//	iterate over associated volumes of the first vertex and count
//	the number of marked vertices it contains.
	VertexBase* vrt = fd.vertex(0);
	VolumeIterator iterEnd = grid.associated_volumes_end(vrt);
	for(VolumeIterator iter = grid.associated_volumes_begin(vrt);
		iter != iterEnd; ++iter)
	{
		Volume* vol = *iter;
		if(vol != v){
			int count = 0;
			uint numVrts = vol->num_vertices();
			for(uint i = 0; i < numVrts; ++i){
				if(grid.is_marked(vol->vertex(i)))
					++count;
			}

		//	if the number of marked vertices in vol matches the
		//	number of vertices of the specified side, we consider
		//	the volume to be a neighbout of that side.
			if(count == numFaceVrts)
				vVolsOut.push_back(vol);
		}
	}

	grid.end_marking();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	CalculateMinTetrahedronHeight - mstepnie
number CalculateMinTetrahedronHeight(const vector3& a, const vector3& b, 
									 const vector3& c, const vector3& d)
{
	number minHeight, tmpMinHeight;

//	Assume a tetrahedron with vertices a, b, c, d. Calculate its direction vectors
	vector3 ab;
	vector3 ac;
	vector3 ad;
	vector3 bd;
	vector3 bc;

	VecSubtract(ab, b, a);
	VecSubtract(ac, c, a);
	VecSubtract(ad, d, a);
	VecSubtract(bd, d, b);
	VecSubtract(bc, c, b);

//	calculate the 4 face normals
	vector3 nabc;
	vector3 nabd;
	vector3 nacd;
	vector3 nbcd;

	VecCross(nabc, ab, ac);
	VecCross(nabd, ad, ab);
	VecCross(nacd, ac, ad);
	VecCross(nbcd, bd, bc);

//	LOTFU�VERFAHREN
	vector3 CutComb;

	///////////
	// FACE ABC
	//	set up matrix for calculating the orthogonal projection of vertex d on face abc
	matrix33 A;
	for(uint i = 0; i<3; ++i)
	{
		A[i][0] = ab[i];
	}
	for(uint i = 0; i<3; ++i)
	{
		A[i][1] = ac[i];
	}
	for(uint i = 0; i<3; ++i)
	{
		A[i][2] = -nabc[i];
	}

	// calculate the height of d respecting abc
	matrix33 A_inv;
	Inverse(A_inv, A);

	vector3 rhs;
	VecSubtract(rhs, d, a);

	MatVecMult(CutComb, A_inv, rhs);
	VecScale(nabc, nabc, CutComb[2]);
	minHeight = VecLength(nabc);


	///////////
	// FACE abd
	//	set up matrix for calculating the orthogonal projection of vertex c on face abd
	for(uint i = 0; i<3; ++i)
	{
		A[i][0] = ad[i];
	}

	for(uint i = 0; i<3; ++i)
	{
		A[i][1] = ab[i];
	}

	for(uint i = 0; i<3; ++i)
	{
		A[i][2] = -nabd[i];
	}

	// calculate the height of d respecting abc
	Inverse(A_inv, A);

	VecSubtract(rhs, c, a);

	MatVecMult(CutComb, A_inv, rhs);
	VecScale(nabd, nabd, CutComb[2]);
	tmpMinHeight = VecLength(nabd);

	if(tmpMinHeight > minHeight)
	{
		minHeight = tmpMinHeight;
	}


	///////////
	// FACE acd
	//	set up matrix for calculating the orthogonal projection of vertex b on face acd
	for(uint i = 0; i<3; ++i)
	{
		A[i][0] = ac[i];
	}
	for(uint i = 0; i<3; ++i)
	{
		A[i][1] = ad[i];
	}
	for(uint i = 0; i<3; ++i)
	{
		A[i][2] = -nacd[i];
	}

	// calculate the height of b respecting acd
	Inverse(A_inv, A);

	VecSubtract(rhs, b, a);

	MatVecMult(CutComb, A_inv, rhs);
	VecScale(nacd, nacd, CutComb[2]);
	tmpMinHeight = VecLength(nacd);

	if(tmpMinHeight < minHeight)
	{
		minHeight = tmpMinHeight;
	}


	///////////
	// FACE bcd
	//	set up matrix for calculating the orthogonal projection of vertex c on face bcd
	for(uint i = 0; i<3; ++i)
	{
		A[i][0] = bd[i];
	}
	for(uint i = 0; i<3; ++i)
	{
		A[i][1] = bc[i];
	}
	for(uint i = 0; i<3; ++i)
	{
		A[i][2] = -nbcd[i];
	}

	// calculate the height of a respecting bcd
	Inverse(A_inv, A);

	VecSubtract(rhs, a, b);

	MatVecMult(CutComb, A_inv, rhs);
	VecScale(nbcd, nbcd, CutComb[2]);
	tmpMinHeight = VecLength(nbcd);

	if(tmpMinHeight < minHeight)
	{
		minHeight = tmpMinHeight;
	}


	return minHeight;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	CalculateMinTetrahedronEdge - mstepnie
number CalculateMaxTetrahedronEdgelength(Grid& grid, Volume& v)
{
	Grid::VertexAttachmentAccessor<AVector3> aaPos(grid, aPosition);
	number maxEdgelength, tmpMaxEdgelength;

//	compare all edges and find shortest
	maxEdgelength = VecDistance(aaPos[v.edge(0).vertex(1)], aaPos[v.edge(0).vertex(0)]);
	for(uint i = 1; i<5; ++i)
	{
		tmpMaxEdgelength = VecDistance(aaPos[v.edge(i).vertex(1)], aaPos[v.edge(i).vertex(0)]);
		if(tmpMaxEdgelength > maxEdgelength)
			maxEdgelength = tmpMaxEdgelength;
	}

	return maxEdgelength;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	CalculateTetrahedronAspectRatio - mstepnie
number CalculateTetrahedronAspectRatio(Grid& grid, Volume& v)
{
	/*
	 * optimal Aspect Ratio of a regular tetrahedron
	 * Q = sqrt(2/3) * a / a = 0.81...
	 */

	Grid::VertexAttachmentAccessor<AVector3> aaPos(grid, aPosition);
	number AspectRatio;
	number maxEdgelength;
	number minTetrahedronHeight;

	maxEdgelength = CalculateMaxTetrahedronEdgelength(grid, v);
	minTetrahedronHeight = CalculateMinTetrahedronHeight(aaPos[v.vertex(0)], aaPos[v.vertex(1)], aaPos[v.vertex(2)], aaPos[v.vertex(3)]);
	AspectRatio = minTetrahedronHeight / maxEdgelength;

	return AspectRatio;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	CalculateTetrahedronVolume - mstepnie
number CalculateTetrahedronVolume(const vector3& a, const vector3& b,
								  const vector3& c, const vector3& d)
{
//
//	Assume a tetrahedron with vertices a, b, c, d, then the volume is given by
//
//	V = 1/6 * |VecDot( (a-d) , VecCross((b-d), (c-d)) )|
//
	number TetrahedronVolume;
	vector3 ad;
	vector3 bd;
	vector3 cd;
	vector3 cross;

	VecSubtract(ad, a, d);
	VecSubtract(bd, b, d);
	VecSubtract(cd, c, d);

	VecCross(cross, bd, cd);

	TetrahedronVolume = VecDot(ad, cross) / 6;

	return TetrahedronVolume;
}

}//	end of namespace



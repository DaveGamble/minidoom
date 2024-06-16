#pragma once

#include <vector>
#include <string>

#include "DataTypes.hpp"
#include "Player.hpp"
#include "ViewRenderer.h"
#include "WADLoader.hpp"

class Map
{
public:
    Map(ViewRenderer *pViewRenderer, const std::string &sName, Player *pPlayer, Things *pThings, WADLoader *w);
	~Map() {}

    void AddVertex(Vertex &v);
    void AddLinedef(WADLinedef &l);
    void AddNode(Node &node);
    void AddSubsector(Subsector &subsector);
    void AddSeg(WADSeg &seg);
    void AddSidedef(WADSidedef &sidedef);
    void AddSector(WADSector &sector);
    void Render3DView();
    void SetLumpIndex(int iIndex);

    int GetPlayerSubSectorHieght();
    int GetXMin();
    int GetXMax();
    int GetYMin();
    int GetYMax();
    int GetLumpIndex();

    Things* GetThings();

protected:
    void BuildSectors();
    void BuildSidedefs();
    void BuildLinedef();
    void BuildSeg();
    void RenderBSPNodes();
    void RenderBSPNodes(int iNodeID);
    void RenderSubsector(int iSubsectorID);

    bool IsPointOnLeftSide(int XPosition, int YPosition, int iNodeID);

    std::vector<Vertex> m_Vertexes;
    std::vector<Sector> m_Sectors;
    std::vector<Sidedef> m_Sidedefs;
    std::vector<Linedef> m_Linedefs;
    std::vector<Seg> m_Segs;
    std::vector<Subsector> m_Subsector;
    std::vector<Node> m_Nodes;


    std::vector<WADSector> m_pSectors;
    std::vector<WADSidedef> m_pSidedefs;
    std::vector<WADLinedef> m_pLinedefs;
    std::vector<WADSeg> m_pSegs;

	int m_XMin {INT_MAX}, m_XMax {INT_MIN}, m_YMin {INT_MAX}, m_YMax {INT_MIN}, m_iLumpIndex {-1};

    Player *m_pPlayer;
    Things *m_pThings;
    ViewRenderer *m_pViewRenderer;
};

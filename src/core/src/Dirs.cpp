#include "Dirs.h"
#include "Fail.h"


namespace MFM {

  SPoint Dirs::FlipSEPointToCorner(const SPoint& pt, const Dir corner)
  {
    SPoint out = pt;
    switch(corner)
    {
    case Dirs::SOUTHEAST: break;
    case Dirs::NORTHEAST: FlipYAxis(out); break;
    case Dirs::SOUTHWEST: FlipXAxis(out); break;
    case Dirs::NORTHWEST: FlipXAxis(out); FlipYAxis(out); break;
    default: FAIL(ILLEGAL_ARGUMENT); break;
    }
    return out;
  }


#if 0
  void Dirs::FillDir(SPoint& pt, u32 dir)
  {
    switch(dir)
    {
    case NORTH:     pt.Set(0, -1); break;
    case NORTHEAST: pt.Set(1,  -1); break;
    case EAST:      pt.Set(1,  0); break;
    case SOUTHEAST: pt.Set(1,   1); break;
    case SOUTH:     pt.Set(0,  1); break;
    case SOUTHWEST: pt.Set(-1,  1); break;
    case WEST:      pt.Set(-1, 0); break;
    case NORTHWEST: pt.Set(-1, -1); break;
    default:
      FAIL(ILLEGAL_ARGUMENT);
    }
  }
#else
  void Dirs::FillDir(SPoint& pt, u32 dir, bool isStaggered)
  {
    if(isStaggered)
      return FillDirStaggered(pt, dir);
    return FillDirCheckerboard(pt, dir);
  }


  void Dirs::FillDirCheckerboard(SPoint& pt, u32 dir)
  {
    switch(dir)
    {
    case NORTH:     pt.Set(0, -1); break;
    case NORTHEAST: pt.Set(1,  -1); break;
    case EAST:      pt.Set(1,  0); break;
    case SOUTHEAST: pt.Set(1,   1); break;
    case SOUTH:     pt.Set(0,  1); break;
    case SOUTHWEST: pt.Set(-1,  1); break;
    case WEST:      pt.Set(-1, 0); break;
    case NORTHWEST: pt.Set(-1, -1); break;
    default:
      FAIL(ILLEGAL_ARGUMENT);
    }
  }

  void Dirs::FillDirStaggered(SPoint& pt, u32 dir)
  {
    //assumes grid width + 1,
    //even rows don't use last spot in row,
    //odd rows don't use first spot in row
    switch(dir)
      {
      case NORTHEAST: pt.Set(1, -1); break; //was 0,-1
      case EAST:      pt.Set(1, 0); break;
      case SOUTHEAST: pt.Set(1, 1); break; //was 0,1
      case SOUTHWEST: pt.Set(-1, 1); break;
      case WEST:      pt.Set(-1, 0); break;
      case NORTHWEST: pt.Set(-1, -1); break;
      case NORTH:
      case SOUTH:
      default:
	FAIL(ILLEGAL_ARGUMENT);
      }
  }
#endif


  u32 Dirs::FromOffset(SPoint& pt)
  {
    switch(pt.GetX())
    {
    case 1:
      switch(pt.GetY())
      {
      case 1:  return SOUTHEAST;
      case 0:  return EAST;
      case -1: return NORTHEAST;
      }
      break;
    case 0:
      switch(pt.GetY())
      {
      case 1:  return SOUTH;
      case -1: return NORTH;
      }
      break;
    case -1:
      switch(pt.GetY())
      {
      case 1:  return SOUTHWEST;
      case 0:  return WEST;
      case -1: return NORTHWEST;
      }
      break;
    }

    FAIL(ILLEGAL_ARGUMENT);
  }
} /* namespace MFM */

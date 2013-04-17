/* 
 * Copyright (c) 2013, Laminar Research.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE.
 *
 */

#include "WED_Validate.h"

#include "WED_Globals.h"

#include "WED_Runway.h"
#include "WED_Sealane.h"
#include "WED_Helipad.h"
#include "WED_Airport.h"
#include "WED_ToolUtils.h"
#include "WED_FacadePlacement.h"
#include "WED_ForestPlacement.h"
#include "WED_ObjPlacement.h"
#include "WED_StringPlacement.h"
#include "WED_LinePlacement.h"
#include "WED_PolygonPlacement.h"
#include "WED_DrapedOrthophoto.h"
#include "WED_FacadeNode.h"
#include "WED_RampPosition.h"
#include "WED_TaxiRoute.h"
#include "WED_ATCFlow.h"
#include "WED_LibraryMgr.h"
#include "WED_AirportBoundary.h"
#include "WED_GISUtils.h"
#include "WED_Group.h"

#include "AptDefs.h"
#include "IResolver.h"

#include "PlatformUtils.h"

#define MAX_LON_SPAN_ROBIN 0.2
#define MAX_LAT_SPAN_ROBIN 0.2


static set<string>	s_used_rwy;
static set<string>	s_used_hel;

static bool GetThingResouce(WED_Thing * who, string& r)
{
	WED_ObjPlacement * obj;
	WED_FacadePlacement * fac;
	WED_ForestPlacement * fst;
	WED_LinePlacement * lin;
	WED_StringPlacement * str;
	WED_PolygonPlacement * pol;
	
	#define CAST_WITH_CHECK(CLASS,VAR) \
	if(who->GetClass() == CLASS::sClass && (VAR = dynamic_cast<CLASS *>(who)) != NULL) { \
		VAR->GetResource(r); \
		return true; } 
	
	CAST_WITH_CHECK(WED_ObjPlacement,obj)
	CAST_WITH_CHECK(WED_FacadePlacement,fac)
	CAST_WITH_CHECK(WED_ForestPlacement,fst)
	CAST_WITH_CHECK(WED_LinePlacement,lin)
	CAST_WITH_CHECK(WED_StringPlacement,str)
	CAST_WITH_CHECK(WED_PolygonPlacement,pol)
	
	return false;
	
	
}

static WED_Thing * ValidateRecursive(WED_Thing * who, WED_LibraryMgr * lib_mgr)
{
	string name, n1, n2;
	string::size_type p;
	int i;
	who->GetName(name);
	string msg;

	//------------------------------------------------------------------------------------
	// CHECKS FOR GENERAL APT.DAT BOGUSNESS
	//------------------------------------------------------------------------------------			
	
	if (who->GetClass() == WED_Runway::sClass || who->GetClass() == WED_Sealane::sClass)
	{
		if (s_used_rwy.count(name))	msg = "The runway/sealane name '" + name + "' has already been used.";
		s_used_rwy.insert(name);
		p = name.find("/");
		if (p != name.npos)
		{
			n1 = name.substr(0,p);
			n2 = name.substr(p+1);
		} else
			n1 = name;

		int suf1 = 0, suf2 = 0;
		int	num1 = -1, num2 = -1;

		if (n1.empty())	msg = "The runway/sealane '" + name + "' has an empty low-end name.";
		else {
			int suffix = n1[n1.length()-1];
			if (suffix < '0' || suffix > '9')
			{
				if (suffix == 'L' || suffix == 'R' || suffix == 'C') suf1 = suffix;
				else msg = "The runway/sealane '" + name + "' has an illegal suffix for the low-end runway.";
				n1.erase(n1.length()-1);
			}

			for (i = 0; i < n1.length(); ++i)
			if (n1[i] < '0' || n1[i] > '9')
			{
				msg = "The runway/sealane '" + name + "' has an illegal characters in its low-end name.";
				break;
			}
			if (i == n1.length())
			{
				num1 = atoi(n1.c_str());
			}
			if (num1 < 1 || num1 > 36)
			{
				msg = "The runway/sealane '" + name + "' has an illegal low-end number, which must be between 1 and 36.";
				num1 = -1;
			}
		}

		if (p != name.npos)
		{
			if (n2.empty())	msg = "The runway/sealane '" + name + "' has an empty high-end name.";
			else {
				int suffix = n2[n2.length()-1];
				if (suffix < '0' || suffix > '9')
				{
					if (suffix == 'L' || suffix == 'R' || suffix == 'C') suf2 = suffix;
					else msg = "The runway/sealane '" + name + "' has an illegal suffix for the high-end runway.";
					n2.erase(n2.length()-1);
				}

				for (i = 0; i < n2.length(); ++i)
				if (n2[i] < '0' || n2[i] > '9')
				{
					msg = "The runway/sealane '" + name + "' has an illegal characters in its high-end name.";
					break;
				}
				if (i == n2.length())
				{
					num2 = atoi(n2.c_str());
				}
				if (num2 < 19 || num2 > 36)
				{
					msg = "The runway/sealane '" + name + "' has an illegal high-end number, which must be between 19 and 36.";
					num2 = -1;
				}
			}
		}

		if (suf1 != 0 && suf2 != 0)
		{
			if ((suf1 == 'L' && suf2 != 'R') ||
				(suf1 == 'R' && suf2 != 'L') ||
				(suf1 == 'C' && suf2 != 'C'))
					msg = "The runway/sealane '" + name + "' has mismatched suffixes - check L vs R, etc.";
		}
		if (num1 != -1 && num2 != -1)
		{
			if (num2 < num1)
				msg = "The runway/sealane '" + name + "' has mismatched runway numbers - the low number must be first.'";
			else if (num2 != num1 + 18)
				msg = "The runway/sealane '" + name + "' has mismatched runway numbers - high end is not the reciprocal of the low-end.";
		}

		if (msg.empty())
		{
			WED_GISLine_Width * lw = dynamic_cast<WED_GISLine_Width *>(who);
			if (lw->GetWidth() < 1.0) msg = "The runway/sealane '" + name + "' must be at least one meter wide.";

			WED_Runway * rwy = dynamic_cast<WED_Runway *>(who);
			if (rwy)
			{
				if (rwy->GetDisp1() + rwy->GetDisp2() > rwy->GetLength()) msg = "The runway/sealane '" + name + "' has overlapping displaced threshholds.";
			}
		}
	}
	if (who->GetClass() == WED_Helipad::sClass)
	{
		if (s_used_hel.count(name))	msg = "The helipad name '" + name + "' has already been used.";
		s_used_hel.insert(name);

		n1 = name;
		if (n1.empty())	msg = "The selected helipad has no name.";
		else {
			if (n1[0] != 'H')	msg = "The helipad '" + name + "' does not start with the letter H.";
			else {
				n1.erase(0,1);
				for (int i = 0; i < n1.length(); ++i)
				{
					if (n1[i] < '0' || n1[i] > '9')
					{
						msg = "The helipad '" + name + "' conntains illegal characters in its name.  It must be in the form H<number>.";
						break;
					}
				}
			}
		}
		if (msg.empty())
		{
			WED_Helipad * heli = dynamic_cast<WED_Helipad *>(who);
			if (heli->GetWidth() < 1.0) msg = "The helipad '" + name + "' is less than one meter wide.";
			if (heli->GetLength() < 1.0) msg = "The helipad '" + name + "' is less than one meter long.";
		}
	}
	if(who->GetClass() == WED_Airport::sClass)
	{
		s_used_hel.clear();
		s_used_rwy.clear();
	}
	
	//------------------------------------------------------------------------------------
	// CHECKS FOR V10 DSF OVERLAY EXTENSIONS
	//------------------------------------------------------------------------------------		
	
	if(who->GetClass() == WED_FacadePlacement::sClass)
	{
		WED_FacadePlacement * fac = dynamic_cast<WED_FacadePlacement*>(who);
		DebugAssert(who);
		if(gExportTarget == wet_xplane_900 && fac->HasCustomWalls())
		{
			msg = "Custom facade wall choices are only supported in X-Plane 10 and newer.";
		}
		
		if(fac->GetNumHoles() > 0)
		{
			msg = "Facades may not have holes in them.";
		}
		
		if(gExportTarget == wet_xplane_900 && WED_HasBezierPol(fac))
			msg = "Curved facades are only supported in X-Plane 10 and newer.";

	}
	
	if(who->GetClass() == WED_ForestPlacement::sClass)
	{
		WED_ForestPlacement * fst = dynamic_cast<WED_ForestPlacement *>(who);
		DebugAssert(fst);
		if(gExportTarget == wet_xplane_900 && fst->GetFillMode() != dsf_fill_area)
			msg = "Line and point forests are only supported in X-Plane 10 and newer.";
	}
	
	//------------------------------------------------------------------------------------
	// CHECKS FOR V10 APT.DAT FEATURES
	//------------------------------------------------------------------------------------	
	
	if(who->GetClass() == WED_RampPosition::sClass)
	{
		AptGate_t	g;
		WED_RampPosition * ramp = dynamic_cast<WED_RampPosition*>(who);
		DebugAssert(ramp);
		ramp->Export(g);
		
		if(gExportTarget == wet_xplane_900)
		if(g.equipment != 0)
		if(g.type != atc_ramp_misc || g.equipment != atc_traffic_all)
			msg = "Gates with specific traffic and types are only suported in X-Plane 10 and newer.";
	}

	if(gExportTarget == wet_xplane_900)
	{
		if(who->GetClass() == WED_ATCFlow::sClass)
		{
			msg = "ATC flow information is only supported in x-Plane 10 and newer.";
		}
		if(who->GetClass() == WED_TaxiRoute::sClass)
		{
			msg = "ATC taxi routes are only supported in x-Plane 10 and newer.";
		}
	}
	
	//------------------------------------------------------------------------------------
	// CHECKS FOR SUBMISSION TO ROBIN
	//------------------------------------------------------------------------------------

	if(gExportTarget == wet_robin)
	{
		if(who->GetClass() != WED_Group::sClass)
		if(WED_GetParentAirport(who) == NULL)
			msg = "You cannot export airport overlays to Robin if overlay elements are outside airports in the hierarchy.";

		if(who->GetClass() == WED_Airport::sClass)
		{
			WED_Airport * apt = dynamic_cast<WED_Airport *>(who);
			Bbox2 bounds;
			apt->GetBounds(gis_Geo, bounds);
			if(bounds.xspan() > MAX_LON_SPAN_ROBIN ||
			   bounds.yspan() > MAX_LAT_SPAN_ROBIN)
			{
				msg = "This airport is too big.  Perhaps a random part of the airport has been dragged to another part of the world?";
			}
		
		}

	
		if(who->GetClass() == WED_AirportBoundary::sClass)
		{
			if(WED_HasBezierPol(dynamic_cast<WED_AirportBoundary*>(who)))
				msg = "Do not use bezier curves in airport boundaries.";
		}

		if(who->GetClass() == WED_DrapedOrthophoto::sClass)
			msg = "Draped orthophotos are not allowed in the global airport database.";

		string res;
		if(GetThingResouce(who,res))
		{
			if(!lib_mgr->IsResourceDefault(res))
				msg = "The library path '" + res + "' is not part of X-Plane's default installation and cannot be submitted to the global airport database.";
		}
	}




	//------------------------------------------------------------------------------------

	if (!msg.empty())
	{
		DoUserAlert(msg.c_str());
		return who;
	}

	int nn = who->CountChildren();
	for (int n = 0; n < nn; ++n)
	{
		WED_Thing * fail = ValidateRecursive(who->GetNthChild(n), lib_mgr);
		if (fail) return fail;
	}
	return NULL;
}

bool	WED_ValidateApt(IResolver * resolver)
{
	s_used_hel.clear();
	s_used_rwy.clear();
	WED_Thing * wrl = WED_GetWorld(resolver);
	ISelection * sel = WED_GetSelect(resolver);
	
	WED_LibraryMgr * lib_mgr = 	SAFE_CAST(WED_LibraryMgr,resolver->Resolver_Find("libmgr"));
	
	WED_Thing * bad_guy = ValidateRecursive(wrl, lib_mgr);
	if (bad_guy)
	{
		wrl->StartOperation("Select Invalid");
		sel->Select(bad_guy);
		wrl->CommitOperation();
		return false;
	}
	return true;
}
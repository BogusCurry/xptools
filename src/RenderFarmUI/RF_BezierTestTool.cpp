/*
 * Copyright (c) 2004, Laminar Research.
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
#include "RF_BezierTestTool.h"

#include "RF_MapTool.h"
#include "RF_MapZoomer.h"
#include "RF_Notify.h"
#include "RF_Msgs.h"
#include "RF_Globals.h"
#include "RF_Progress.h"
#include "MapAlgs.h"
#include "UIUtils.h"
#include "AssertUtils.h"
#include "RF_DrawMap.h"
#include "XPLMGraphics.h"

#if APL
	#include <OpenGL/gl.h>
#else
	#include <gl.h>
#endif

static const DragHandleInfo_t kHandleInfos[8] = {
	0,0,1,1,
	0,0,1,1,
	0,0,1,1,
	0,0,1,1,
	0,0,1,1,
	0,0,1,1,
	0,0,1,1,
	0,0,1,1 };

//static int self_intersect_recursive(const Bezier2& lhs, const Bezier2& rhs, int d, Bbox2& b1, Bbox2& b2);
//static int	intersect_recursive(const Bezier2& lhs, const Bezier2& rhs, int d, Bbox2& b1, Bbox2& b2);




RF_BezierTestTool::RF_BezierTestTool(RF_MapZoomer * inZoomer) :
	RF_MapTool(inZoomer),
	mHandles(8,kHandleInfos,4,this),
	mSegs(200),
	mSplit(0.5)
{
}

RF_BezierTestTool::~RF_BezierTestTool()
{
}

void	RF_BezierTestTool::DrawFeedbackUnderlay(
							bool				inCurrent)
{
}

void	RF_BezierTestTool::DrawFeedbackOverlay(
							bool				inCurrent)
{
	if (inCurrent)
	{
		XPLMSetGraphicsState(0, 0, 0,    0, 0,  0, 0);
		glColor3f(0.0, 1.0, 0.3);
		glBegin(GL_QUADS);
		for (int n = 0; n < 8; ++n)
			mHandles.DrawHandle(n);
		glEnd();
		glBegin(GL_LINES);
		mHandles.ConnectHandle(0, 1);
		mHandles.ConnectHandle(2, 3);
		mHandles.ConnectHandle(4, 5);
		mHandles.ConnectHandle(6, 7);
		glEnd();

		Bezier2 a(mBezier1),b(mBezier2);

		int i = a.intersect(b,10);

		glColor3f(i || a.self_intersect(5) ? 1 : 0, i ? 0 : 1, 0);

		int mx,my;
		XPLMGetMouseLocation(&mx,&my);
		if (a.is_near(Point2(GetZoomer()->XPixelToLon(mx),GetZoomer()->YPixelToLat(my)),0.001))
			glColor3f(1,1,1);

		glBegin(GL_LINE_STRIP);
		for (int n = 0; n <= mSegs; ++n)
		{
			Point2 p = a.midpoint((float) n / (float) mSegs);
			glVertex2f(GetZoomer()->LonToXPixel(p.x),GetZoomer()->LatToYPixel(p.y));
		}
		glEnd();

		double ts[2] = { 3.0, 3.0 };
		int y_change = b.y_monotone_regions(ts);

		glBegin(GL_LINE_STRIP);
		for (int n = 0; n <= mSegs; ++n)
		{
			double t = (double) n / (double) mSegs;
							glColor3f(0,1,0);
			if (t < ts[1])	glColor3f(1,0,0);
			if (t < ts[0])	glColor3f(0,0,1);

			Point2 p = b.midpoint(t);
			glVertex2f(GetZoomer()->LonToXPixel(p.x),GetZoomer()->LatToYPixel(p.y));
		}
		glEnd();

		Bbox2	f1,f2;
		a.bounds(f1);
		b.bounds(f2);
		glColor3f(0.3,0.3,0.3);
		glBegin(GL_LINE_LOOP);
			glVertex2f(GetZoomer()->LonToXPixel(f1.p1.x),GetZoomer()->LatToYPixel(f1.p1.y));
			glVertex2f(GetZoomer()->LonToXPixel(f1.p1.x),GetZoomer()->LatToYPixel(f1.p2.y));
			glVertex2f(GetZoomer()->LonToXPixel(f1.p2.x),GetZoomer()->LatToYPixel(f1.p2.y));
			glVertex2f(GetZoomer()->LonToXPixel(f1.p2.x),GetZoomer()->LatToYPixel(f1.p1.y));
		glEnd();

		glBegin(GL_LINE_LOOP);
			glVertex2f(GetZoomer()->LonToXPixel(f2.p1.x),GetZoomer()->LatToYPixel(f2.p1.y));
			glVertex2f(GetZoomer()->LonToXPixel(f2.p1.x),GetZoomer()->LatToYPixel(f2.p2.y));
			glVertex2f(GetZoomer()->LonToXPixel(f2.p2.x),GetZoomer()->LatToYPixel(f2.p2.y));
			glVertex2f(GetZoomer()->LonToXPixel(f2.p2.x),GetZoomer()->LatToYPixel(f2.p1.y));
		glEnd();

		glColor4f(1,1,1,0.3);
		double bs[4];
		GetZoomer()->GetPixelBounds(bs[0],bs[1],bs[2],bs[3]);
		double x = GetZoomer()->XPixelToLon(mx);
		double y = GetZoomer()->YPixelToLat(my);

		list<Bezier2> l;
		l.push_back(b);
		l.push_back(Bezier2(b.p2,b.p2,b.p1,b.p1));
		if (inside_polygon_bez(l.begin(),l.end(),Point2(x,y)))
			glColor3f(1,0,0);

		glBegin(GL_LINES);
		glVertex2f(bs[0], my);
		glVertex2f(bs[2], my);
		glVertex2f(mx,bs[1]);
		glVertex2f(mx,bs[3]);
		glEnd();

		glPointSize(5);
		glColor3f(1,1,1);
		glBegin(GL_POINTS);


		if (y_change == 0)
		{
			Bezier2 b1(b);

			b1.c1 = b1.p1;
			b1.c2 = b1.p2;

			glColor3f(0,0,1);
//			if ((x >= b1.p1.x && x <= b1.p2.x) || (x >= b1.p2.x && x <= b1.p2.y))
//			glVertex2f(mx,GetZoomer()->LatToYPixel(b1.y_at_x(GetZoomer()->XPixelToLon(mx))));
			if ((y >= b1.p1.y && y <= b1.p2.y) || (y >= b1.p2.y && y <= b1.p1.y))
			glVertex2f(GetZoomer()->LonToXPixel(b1.x_at_y(GetZoomer()->YPixelToLat(my))), my);
		}
		else if (y_change == 1)
		{
			Bezier2	b1,b2;
			b.partition(b1,b2,ts[0]);

			glColor3f(0,0,1);
//			if ((x >= b1.p1.x && x <= b1.p2.x) || (x >= b1.p2.x && x <= b1.p2.y))
//			glVertex2f(mx,GetZoomer()->LatToYPixel(b1.y_at_x(GetZoomer()->XPixelToLon(mx))));
			if ((y >= b1.p1.y && y <= b1.p2.y) || (y >= b1.p2.y && y <= b1.p1.y))
			glVertex2f(GetZoomer()->LonToXPixel(b1.x_at_y(GetZoomer()->YPixelToLat(my))), my);

			glColor3f(1,0,0);
//			if ((x >= b2.p1.x && x <= b2.p2.x) || (x >= b2.p2.x && x <= b2.p2.y))
//			glVertex2f(mx,GetZoomer()->LatToYPixel(b2.y_at_x(GetZoomer()->XPixelToLon(mx))));
			if ((y >= b2.p1.y && y <= b2.p2.y) || (y >= b2.p2.y && y <= b2.p1.y))
			glVertex2f(GetZoomer()->LonToXPixel(b2.x_at_y(GetZoomer()->YPixelToLat(my))), my);
	}
		else if (y_change == 2)
		{

			Bezier2	b1,b2, b3, bp;
			b.partition(b1,bp,ts[0]);
			bp.partition(b2, b3, (ts[1]-ts[0]) / (1 - ts[0]));

			glColor3f(0,0,1);
//			if ((x >= b1.p1.x && x <= b1.p2.x) || (x >= b1.p2.x && x <= b1.p2.y))
//			glVertex2f(mx,GetZoomer()->LatToYPixel(b1.y_at_x(GetZoomer()->XPixelToLon(mx))));
			if ((y >= b1.p1.y && y <= b1.p2.y) || (y >= b1.p2.y && y <= b1.p1.y))
			glVertex2f(GetZoomer()->LonToXPixel(b1.x_at_y(GetZoomer()->YPixelToLat(my))), my);

			glColor3f(1,0,0);
//			if ((x >= b2.p1.x && x <= b2.p2.x) || (x >= b2.p2.x && x <= b2.p2.y))
//			glVertex2f(mx,GetZoomer()->LatToYPixel(b2.y_at_x(GetZoomer()->XPixelToLon(mx))));
			if ((y >= b2.p1.y && y <= b2.p2.y) || (y >= b2.p2.y && y <= b2.p1.y))
			glVertex2f(GetZoomer()->LonToXPixel(b2.x_at_y(GetZoomer()->YPixelToLat(my))), my);

			glColor3f(0,1,0);
//			if ((x >= b3.p1.x && x <= b3.p2.x) || (x >= b3.p2.x && x <= b3.p2.y))
//			glVertex2f(mx,GetZoomer()->LatToYPixel(b3.y_at_x(GetZoomer()->XPixelToLon(mx))));
			if ((y >= b3.p1.y && y <= b3.p2.y) || (y >= b3.p2.y && y <= b3.p1.y))
			glVertex2f(GetZoomer()->LonToXPixel(b3.x_at_y(GetZoomer()->YPixelToLat(my))), my);

		}
		glEnd();
	}
}

bool	RF_BezierTestTool::HandleClick(
							XPLMMouseStatus		inStatus,
							int 				inX,
							int 				inY,
							int 				inButton)
{
	if (inButton != 0) return false;
	switch(inStatus) {
	case xplm_MouseDown:
		return mHandles.StartDrag(inX, inY);
	case xplm_MouseDrag:
		mHandles.ContinueDrag(inX, inY);
		return true;
	case xplm_MouseUp:
		mHandles.EndDrag(inX, inY);
		return true;
	}
	return false;
}

#pragma mark -

int		RF_BezierTestTool::GetNumProperties(void)
{
	return 2;
}

void	RF_BezierTestTool::GetNthPropertyName(int n, string& s)
{
	switch(n) {
	case 0: s = "Segments:";	break;
	case 1: s = "Split:";		break;
	}
}

double	RF_BezierTestTool::GetNthPropertyValue(int n)
{
	switch(n) {
	case 0: return mSegs;
	case 1: return mSplit;
	default: return 0;
	}
}

void	RF_BezierTestTool::SetNthPropertyValue(int n, double v)
{
	switch(n) {
	case 0: mSegs = v;	break;
	case 1: mSplit = v;	break;
	}
}

#pragma mark -

int		RF_BezierTestTool::GetNumButtons(void)
{
	return 1;
}

void	RF_BezierTestTool::GetNthButtonName(int n, string& s)
{
	switch(n) {
	case 0: s = "Reset";	break;
	}
}

void	RF_BezierTestTool::NthButtonPressed(int n)
{
	double bounds[4];
	switch(n) {
	case 0:
		GetZoomer()->GetMapVisibleBounds(bounds[0], bounds[1], bounds[2], bounds[3]);
		mBezier1.p1=Point2(bounds[0],bounds[1]);
		mBezier1.c1=Point2(bounds[0],bounds[3]);
		mBezier1.c2=Point2(bounds[2],bounds[3]);
		mBezier1.p2=Point2(bounds[2],bounds[1]);
		mBezier2.c1=Point2(bounds[0],bounds[1]);
		mBezier2.p1=Point2(bounds[0],bounds[3]);
		mBezier2.p2=Point2(bounds[2],bounds[3]);
		mBezier2.c2=Point2(bounds[2],bounds[1]);
		break;
	}
}

char *	RF_BezierTestTool::GetStatusText(void)
{
	static char buf[512];
	double d[4];
	int n = mBezier1.monotone_regions(d);
	sprintf(buf,"(%d: %lf,%lf,%lf,%lf)",
			n,d[0],d[1],d[2],d[3]);
	return buf;
}

#pragma mark -

double		RF_BezierTestTool::UIToLogX(double v) const
{
	return GetZoomer()->XPixelToLon(v);
}

double		RF_BezierTestTool::UIToLogY(double v) const
{
	return GetZoomer()->YPixelToLat(v);
}

double		RF_BezierTestTool::LogToUIX(double v) const
{
	return GetZoomer()->LonToXPixel(v);
}

double		RF_BezierTestTool::LogToUIY(double v) const
{
	return GetZoomer()->LatToYPixel(v);
}

double		RF_BezierTestTool::GetHandleX(int inHandle) const
{
	switch(inHandle) {
	case 0:	return mBezier1.p1.x;
	case 1:	return mBezier1.c1.x;
	case 2:	return mBezier1.c2.x;
	case 3:	return mBezier1.p2.x;
	case 4:	return mBezier2.p1.x;
	case 5:	return mBezier2.c1.x;
	case 6:	return mBezier2.c2.x;
	case 7:	return mBezier2.p2.x;
	default: return 0.0;
	}
}

double		RF_BezierTestTool::GetHandleY(int inHandle) const
{
	switch(inHandle) {
	case 0:	return mBezier1.p1.y;
	case 1:	return mBezier1.c1.y;
	case 2:	return mBezier1.c2.y;
	case 3:	return mBezier1.p2.y;
	case 4:	return mBezier2.p1.y;
	case 5:	return mBezier2.c1.y;
	case 6:	return mBezier2.c2.y;
	case 7:	return mBezier2.p2.y;
	default: return 0.0;
	}
}

void		RF_BezierTestTool::MoveHandleX(int handle, double deltaX)
{
	switch(handle) {
	case 0:	mBezier1.p1.x += deltaX; break;
	case 1:	mBezier1.c1.x += deltaX; break;
	case 2:	mBezier1.c2.x += deltaX; break;
	case 3:	mBezier1.p2.x += deltaX; break;
	case 4:	mBezier2.p1.x += deltaX; break;
	case 5:	mBezier2.c1.x += deltaX; break;
	case 6:	mBezier2.c2.x += deltaX; break;
	case 7:	mBezier2.p2.x += deltaX; break;
	}
}

void		RF_BezierTestTool::MoveHandleY(int handle, double deltaY)
{
	switch(handle) {
	case 0:	mBezier1.p1.y += deltaY; break;
	case 1:	mBezier1.c1.y += deltaY; break;
	case 2:	mBezier1.c2.y += deltaY; break;
	case 3:	mBezier1.p2.y += deltaY; break;
	case 4:	mBezier2.p1.y += deltaY; break;
	case 5:	mBezier2.c1.y += deltaY; break;
	case 6:	mBezier2.c2.y += deltaY; break;
	case 7:	mBezier2.p2.y += deltaY; break;
	}
}


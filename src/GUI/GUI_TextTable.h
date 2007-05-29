#ifndef GUI_TEXTTABLE_H
#define GUI_TEXTTABLE_H

#include "GUI_Commander.h"

class	GUI_Pane;
class	GUI_TextField;

/*

	GUI_TextTable - THEORY OF OPERATION
	
	The text table classes provide a basic set of behavior for tables and their headers for dealing
	with text data.  They have a further set of plugin behaviors that provide the text content,
	and a series of structs that are used to transfer that content.

*/

#include "GUI_Listener.h"
#include "GUI_Broadcaster.h"

#include "GUI_Table.h"

// Cell content as known by a text table - we have a few different kinds of cell
// displays...see comments for which fields they use.

enum GUI_CellContentType {
								// GET					SET
	gui_Cell_None,
	gui_Cell_Disclose,			// n/a - this is used as an internal symbol for disclosure tris
	gui_Cell_EditText,			// string				string
	gui_Cell_CheckBox,			// int val				int val
	gui_Cell_Integer,			// int val				int val
	gui_Cell_Double,			// double val			double val
	gui_Cell_Enum,				// string&int			int
	gui_Cell_EnumSet			// string val&int set	int set
};


struct	GUI_CellContent {
	GUI_CellContentType		content_type;
	int						can_edit;
	int						can_disclose;
	int						can_select;
	int						can_drag;
	
	int						is_disclosed;
	int						is_selected;
	int						indent_level;

	string					text_val;		// Only one of these is used - which one depends on the cell content type!
	int						int_val;
	double					double_val;
	set<int>				int_set_val;
};

struct GUI_HeaderContent {
	string					title;
	int						is_selected;
	
	int						can_resize;
	int						can_select;
};

typedef	map<int, string>		GUI_EnumDictionary;

class	GUI_TextTableProvider {
public:

	virtual void	GetCellContent(
						int							cell_x, 
						int							cell_y, 
						GUI_CellContent&			the_content)=0;	
	virtual	void	GetEnumDictionary(
						int							cell_x, 
						int							cell_y, 
						GUI_EnumDictionary&			out_dictionary)=0;
	virtual	void	AcceptEdit(
						int							cell_x,
						int							cell_y,
						const GUI_CellContent&		the_content,
						int							apply_all)=0;
	virtual	void	ToggleDisclose(
						int							cell_x,
						int							cell_y)=0;
	virtual	void	DoDrag(
						GUI_Pane *					drag_emitter,
						int							mouse_x,
						int							mouse_y,
						int							bounds[4])=0;

	virtual void	SelectionStart(
						int							clear)=0;
	virtual	int		SelectGetExtent(
						int&						low_x,
						int&						low_y,
						int&						high_x,
						int&						high_y)=0;
	virtual	int		SelectGetLimits(
						int&						low_x,
						int&						low_y,
						int&						high_x,
						int&						high_y)=0;
	virtual	void	SelectRange(
						int							start_x,
						int							start_y,
						int							end_x,
						int							end_y,
						int							is_toggle)=0;
	virtual	void	SelectionEnd(void)=0;

	virtual	int		TabAdvance(
						int&						io_x,
						int&						io_y,
						int							reverse,
						GUI_CellContent&			the_content)=0;

	virtual	void					GetLegalDropOperations(
											int&						allow_between_col,
											int&						allow_between_row,
											int&						allow_into_cell)=0;						
	virtual	GUI_DragOperation		CanDropIntoCell(
											int							cell_x,
											int							cell_y,
											GUI_DragData *				drag, 
											GUI_DragOperation			allowed, 
											GUI_DragOperation			recommended,
											int&						whole_col,
											int&						whole_row)=0;
	virtual	GUI_DragOperation		CanDropBetweenColumns(
											int							cell_x,
											GUI_DragData *				drag, 
											GUI_DragOperation			allowed, 
											GUI_DragOperation			recommended)=0;
	virtual	GUI_DragOperation		CanDropBetweenRows(
											int							cell_y,
											GUI_DragData *				drag, 
											GUI_DragOperation			allowed, 
											GUI_DragOperation			recommended)=0;


	virtual	GUI_DragOperation		DoDropIntoCell(
											int							cell_x,
											int							cell_y,
											GUI_DragData *				drag, 
											GUI_DragOperation			allowed, 
											GUI_DragOperation			recommended)=0;
	virtual	GUI_DragOperation		DoDropBetweenColumns(
											int							cell_x,
											GUI_DragData *				drag, 
											GUI_DragOperation			allowed, 
											GUI_DragOperation			recommended)=0;
	virtual	GUI_DragOperation		DoDropBetweenRows(
											int							cell_y,
											GUI_DragData *				drag, 
											GUI_DragOperation			allowed, 
											GUI_DragOperation			recommended)=0;


};

class	GUI_TextTableHeaderProvider { 
public:

	virtual	void	GetHeaderContent(
						int							cell_x,
						GUI_HeaderContent&			the_content)=0;
						
};

// A text table is table content - that is, the drawing brains of a table.
// It in turn queries the "provider" for the actual content.  It allows you to specify a table as strings (easy)
// instead of drawing calls (PITA).

class	GUI_TextTable : public GUI_TableContent, public GUI_Broadcaster, public GUI_Commander {
public:

						 GUI_TextTable(GUI_Commander * parent);
	virtual				~GUI_TextTable();
	
			void		SetProvider(GUI_TextTableProvider * content);
			void		SetParentTable(GUI_Table * parent);

	virtual	void		CellDraw	 (int cell_bounds[4], int cell_x, int cell_y, GUI_GraphState * inState);
	virtual	int			CellMouseDown(int cell_bounds[4], int cell_x, int cell_y, int mouse_x, int mouse_y, int button, GUI_KeyFlags flags, int& want_lock);
	virtual	void		CellMouseDrag(int cell_bounds[4], int cell_x, int cell_y, int mouse_x, int mouse_y, int button									  );
	virtual	void		CellMouseUp  (int cell_bounds[4], int cell_x, int cell_y, int mouse_x, int mouse_y, int button									  );
	virtual	GUI_DragOperation	CellDragEnter	(int cell_bounds[4], int cell_x, int cell_y, int mouse_x, int mouse_y, GUI_DragData * drag, GUI_DragOperation allowed, GUI_DragOperation recommended);
	virtual	GUI_DragOperation	CellDragWithin	(int cell_bounds[4], int cell_x, int cell_y, int mouse_x, int mouse_y, GUI_DragData * drag, GUI_DragOperation allowed, GUI_DragOperation recommended);
	virtual	void				CellDragLeave	(int cell_bounds[4], int cell_x, int cell_y);
	virtual	GUI_DragOperation	CellDrop		(int cell_bounds[4], int cell_x, int cell_y, int mouse_x, int mouse_y, GUI_DragData * drag, GUI_DragOperation allowed, GUI_DragOperation recommended);
	virtual	void		Deactivate(void);

	virtual	int			KeyPress(char inKey, int inVK, GUI_KeyFlags inFlags);
	virtual	int			AcceptTakeFocus(void) 	{ return 1; }

private:

	enum GUI_DragPart {			// DRAG PARTS - divide the cell into 4 zones, for the uppe rand lower half, and closer to the center or edges.
		drag_LowerOrInto,		// If the cell doesn't support reordering, we use "whole cell".
		drag_IntoOrLower,
		drag_IntoOrHigher,
		drag_HigherOrInto,
		drag_WholeCell
	};

	enum GUI_DragTableDest {	// Where we expect the row to go:
		gui_Table_None,					// No drag
		gui_Table_Row,					// Entire row at once - INTO cell
		gui_Table_Column,				// Entire col at once - INTO cell
		gui_Table_Cell,					// Just the cell! - INTO cell
		gui_Insert_Left,				// INsertions - BETWEEN cells, based on this position relative to the insert cell.
		gui_Insert_Right,
		gui_Insert_Bottom,
		gui_Insert_Top
	};

			void			CreateEdit(int cell_bounds[4]);
			int				TerminateEdit(bool inSave, bool inAll);			
			GUI_DragPart	GetCellDragPart(int cell_bounds[4], int x, int y, int vertical);
					
	GUI_TextTableProvider * mContent;
	int						mClickCellX;
	int						mClickCellY;
	GUI_CellContent			mEditInfo;
	int						mInBounds;
	int						mTrackLeft;
	int						mTrackRight;
	GUI_Table *				mParent;
	GUI_TextField *			mTextField;
	
	GUI_KeyFlags			mModifiers;
	int						mSelStartX;
	int						mSelStartY;
	
	
	GUI_DragTableDest		mDragDest;
	int						mDragX;
	int						mDragY;
	GUI_DragPart			mDragPart;
	GUI_DragOperation		mLastOp;
};	



class	GUI_TextTableHeader : public GUI_TableHeader, public GUI_Broadcaster {
public:
						 GUI_TextTableHeader();
	virtual				~GUI_TextTableHeader();
	
			void		SetProvider(GUI_TextTableHeaderProvider * content);
			void		SetGeometry(GUI_TableGeometry * geometry);

	virtual	void		HeadDraw	 (int cell_bounds[4], int cell_x, GUI_GraphState * inState);
	virtual	int			HeadMouseDown(int cell_bounds[4], int cell_x, int mouse_x, int mouse_y, int button, GUI_KeyFlags flags, int& want_lock);
	virtual	void		HeadMouseDrag(int cell_bounds[4], int cell_x, int mouse_x, int mouse_y, int button									  );
	virtual	void		HeadMouseUp  (int cell_bounds[4], int cell_x, int mouse_x, int mouse_y, int button									  );

private:
	GUI_TextTableHeaderProvider *	mContent;
	GUI_TableGeometry *				mGeometry;
	int								mCellResize;
	int								mLastX;

};

class	GUI_TextTableSide : public GUI_TableSide, public GUI_Broadcaster {
public:
						 GUI_TextTableSide();
	virtual				~GUI_TextTableSide();
	
			void		SetProvider(GUI_TextTableHeaderProvider * content);
			void		SetGeometry(GUI_TableGeometry * geometry);

	virtual	void		SideDraw	 (int cell_bounds[4], int cell_y, GUI_GraphState * inState);
	virtual	int			SideMouseDown(int cell_bounds[4], int cell_y, int mouse_x, int mouse_y, int button, GUI_KeyFlags flags, int& want_lock);
	virtual	void		SideMouseDrag(int cell_bounds[4], int cell_y, int mouse_x, int mouse_y, int button									   );
	virtual	void		SideMouseUp  (int cell_bounds[4], int cell_y, int mouse_x, int mouse_y, int button									   );

private:
	GUI_TextTableHeaderProvider *	mContent;
	GUI_TableGeometry *				mGeometry;
	int								mCellResize;
	int								mLastY;

};



#endif /* GUI_TEXTTABLE_H */
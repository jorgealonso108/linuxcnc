/* Classic Ladder Project */
/* Copyright (C) 2001-2007 Marc Le Douarain */
/* http://membres.lycos.fr/mavati/classicladder/ */
/* http://www.sourceforge.net/projects/classicladder */
/* February 2001 */
/* ---------------------------- */
/* GTK Interface & Main */
/* Inspired (at the start) from the scribble example. */
/* ---------------------------------- */
/* This library is free software; you can redistribute it and/or */
/* modify it under the terms of the GNU Lesser General Public */
/* License as published by the Free Software Foundation; either */
/* version 2.1 of the License, or (at your option) any later version. */

/* This library is distributed in the hope that it will be useful, */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU */
/* Lesser General Public License for more details. */

/* You should have received a copy of the GNU Lesser General Public */
/* License along with this library; if not, write to the Free Software */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "classicladder.h"
#include "global.h"
#include "classicladder_gtk.h"
//#include "hardware.h"

GdkPixmap *pixmap = NULL;
GtkWidget *drawing_area = NULL;
GtkWidget *entrylabel,*entrycomment;
GtkWidget *CheckDispSymbols;
#if defined( RT_SUPPORT ) || defined( __XENO__ )
GtkWidget *DurationOfLastScan;
#endif
GtkWidget *ButtonRunStop;
GtkWidget *VScrollBar;
GtkWidget *HScrollBar;
GtkAdjustment * AdjustVScrollBar;
GtkAdjustment * AdjustHScrollBar;
GtkWidget *FileSelector;
GtkWidget *ConfirmDialog;
GtkWidget *RungWindow;
GtkWidget *StatusBar;
gint StatusBarContextId;

#include "drawing.h"
#include "vars_access.h"
#include "calc.h"
#include "files_project.h"
#include "edit.h"
#include "edit_gtk.h"
#include "editproperties_gtk.h"
#include "manager_gtk.h"
#include "config_gtk.h"
#include "socket_server.h"
#include "socket_modbus_master.h"
#ifdef SEQUENTIAL_SUPPORT
#include "calc_sequential.h"
#endif
#ifdef GNOME_PRINT_USE
#include "print_gnome.h"
#endif
#include "symbols_gtk.h"
#include "spy_vars_gtk.h"

/* Create a new backing pixmap of the appropriate size */
static gint configure_event( GtkWidget         *widget,
                            GdkEventConfigure *event )
{
	if (pixmap)
		gdk_pixmap_unref(pixmap);

	pixmap = gdk_pixmap_new(widget->window,
							widget->allocation.width,
							widget->allocation.height,
							-1);
	gdk_draw_rectangle (pixmap,
						widget->style->white_gc,
						TRUE,
						0, 0,
						widget->allocation.width,
						widget->allocation.height);
	return TRUE;
}

/* Redraw the screen from the backing pixmap */
static gint expose_event( GtkWidget      *widget,
                        GdkEventExpose *event )
{
	gdk_draw_pixmap(widget->window,
					widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
					pixmap,
					event->area.x, event->area.y,
					event->area.x, event->area.y,
					event->area.width, event->area.height);

	return FALSE;
}

void UpdateVScrollBar()
{
	int iSecurityBreak = 0;
	int iCurrentLanguage = SectionArray[ InfosGene->CurrentSection ].Language;
	if ( iCurrentLanguage==SECTION_IN_LADDER )
	{
		int NbrRungs = 1;
		int ScanRung = InfosGene->FirstRung;
		int NumCurrentRung = 0;
		while ( ScanRung!=InfosGene->LastRung && iSecurityBreak++<=NBR_RUNGS )
		{
			NbrRungs++;
			ScanRung = RungArray[ ScanRung ].NextRung;
		}
		ScanRung = InfosGene->FirstRung;
		iSecurityBreak = 0;
		while ( ScanRung!=InfosGene->CurrentRung && iSecurityBreak++<=NBR_RUNGS )
		{
			NumCurrentRung++;
			ScanRung = RungArray[ ScanRung ].NextRung;
		}
		if ( iSecurityBreak>=NBR_RUNGS )
			printf("!!!error loop in UpdateVScrollBar()!\n");
//printf("Nbr rungs=%d , NumRung=%d\n", NbrRungs, NumCurrentRung);
		AdjustVScrollBar->lower = 0;
		AdjustVScrollBar->upper = NbrRungs * InfosGene->BlockHeight*RUNG_HEIGHT;
		AdjustVScrollBar->value = NumCurrentRung *  InfosGene->BlockHeight*RUNG_HEIGHT;
		while( AdjustVScrollBar->value+InfosGene->PageHeight > AdjustVScrollBar->upper )
		{
			AdjustVScrollBar->value = AdjustVScrollBar->value - InfosGene->BlockHeight*RUNG_HEIGHT;
		}
		AdjustVScrollBar->step_increment = InfosGene->BlockHeight;
		AdjustVScrollBar->page_increment = InfosGene->BlockHeight*RUNG_HEIGHT;
		AdjustVScrollBar->page_size = InfosGene->PageHeight;
		gtk_adjustment_changed( AdjustVScrollBar );
		gtk_adjustment_value_changed( AdjustVScrollBar );
		gtk_widget_hide( HScrollBar );
//        gtk_widget_show( entrylabel );
//        gtk_widget_show( entrycomment );
	}
#ifdef SEQUENTIAL_SUPPORT
	if ( iCurrentLanguage==SECTION_IN_SEQUENTIAL )
	{
		gtk_widget_show( HScrollBar );
//        gtk_widget_hide( entrylabel );
//        gtk_widget_hide( entrycomment );
		refresh_label_comment( );
		AdjustVScrollBar->lower = 0;
		AdjustVScrollBar->upper = SEQ_PAGE_HEIGHT * SEQ_SIZE_DEF;
		AdjustVScrollBar->value = 0;
		AdjustVScrollBar->step_increment = SEQ_SIZE_DEF;
		AdjustVScrollBar->page_increment = InfosGene->PageHeight;
		AdjustVScrollBar->page_size = InfosGene->PageHeight;
		gtk_adjustment_changed( AdjustVScrollBar );
		gtk_adjustment_value_changed( AdjustVScrollBar );
		AdjustHScrollBar->lower = 0;
		AdjustHScrollBar->upper = SEQ_PAGE_WIDTH * SEQ_SIZE_DEF;
		AdjustHScrollBar->value = 0;
		AdjustHScrollBar->step_increment = SEQ_SIZE_DEF;
		AdjustHScrollBar->page_increment = InfosGene->PageWidth;
		AdjustHScrollBar->page_size = InfosGene->PageWidth;
		gtk_adjustment_changed( AdjustHScrollBar );
		gtk_adjustment_value_changed( AdjustHScrollBar );
	}
#endif
}

void ChoiceOfTheCurrentRung( int NbrOfRungsAfterTopRung )
{
	int DecptNbrRungs = NbrOfRungsAfterTopRung;

//printf("OffsetHiddenTopRungDisplayed=%d\n", InfosGene->OffsetHiddenTopRungDisplayed);
	// per default, the current rung, is the top one displayed...
	InfosGene->CurrentRung = InfosGene->TopRungDisplayed;
	// if OffsetHiddenTopRungDisplayed==0, vertical shift of the current rung is 0,
	// else this is the y value to substract to have the vertical shift... (we will add many full rungs heights after!)
	InfosGene->OffsetCurrentRungDisplayed = -1*InfosGene->OffsetHiddenTopRungDisplayed;

//printf("=> In choice of current rung, %d passes...(CurrentRung=%d)\n",NbrOfRungsAfterTopRung, InfosGene->CurrentRung);
	while( DecptNbrRungs>0 && InfosGene->CurrentRung!=InfosGene->LastRung )
	{
		InfosGene->CurrentRung = RungArray[ InfosGene->CurrentRung ].NextRung;
		InfosGene->OffsetCurrentRungDisplayed += InfosGene->BlockHeight*RUNG_HEIGHT;
		DecptNbrRungs--;
	}

//printf("-> CurrentRung=%d , OffsetCurrentRungDisplayed=%d\n", InfosGene->CurrentRung, InfosGene->OffsetCurrentRungDisplayed);
	if ( InfosGene->OffsetCurrentRungDisplayed<0 )
		printf( "Error in ChoiceOfTheCurrentRung( %d ) with OffsetCurrentRungDisplayed=%d\n", NbrOfRungsAfterTopRung, InfosGene->OffsetCurrentRungDisplayed );
	refresh_label_comment( );
}

static gint VScrollBar_value_changed_event( GtkAdjustment * ScrollBar, void * not_used )
{
	int iCurrentLanguage = SectionArray[ InfosGene->CurrentSection ].Language;
	if ( iCurrentLanguage==SECTION_IN_LADDER )
	{
		int NumRung = ((int)ScrollBar->value)/(InfosGene->BlockHeight*RUNG_HEIGHT);
		int ScanRung = 0;
		InfosGene->TopRungDisplayed = InfosGene->FirstRung;
		if ( NumRung<0 )
			NumRung = 0;
		while( ScanRung!=NumRung )
		{
			InfosGene->TopRungDisplayed = RungArray[ InfosGene->TopRungDisplayed ].NextRung;
			ScanRung++;
		}
		InfosGene->OffsetHiddenTopRungDisplayed = ((int)ScrollBar->value)%(InfosGene->BlockHeight*RUNG_HEIGHT);

		// if top rung displayed entirely (no vertical offset), it's the current rung => give '0'.
		// else, search the next one => give '1'.
		ChoiceOfTheCurrentRung( (InfosGene->OffsetHiddenTopRungDisplayed>0)?1:0 );
	}
	InfosGene->VScrollValue = (int)ScrollBar->value;
	return TRUE;
}
static gint HScrollBar_value_changed_event( GtkAdjustment * ScrollBar, void * not_used )
{
	InfosGene->HScrollValue = (int)ScrollBar->value;
	return TRUE;
}

static gint button_press_event( GtkWidget *widget, GdkEventButton *event )
{
	if (event->button == 1 && pixmap != NULL)
	{
		if (EditDatas.ModeEdit)
		{
			EditElementInThePage(event->x,event->y);
		}
		else
		{
			// we can select the current rung by clicking on one.
			// the current rung is the one that will be modified...
			int iCurrentLanguage = SectionArray[ InfosGene->CurrentSection ].Language;
			if ( iCurrentLanguage==SECTION_IN_LADDER )
			{
				char DoSelection = TRUE;
				if ( InfosGene->OffsetHiddenTopRungDisplayed>0 )
				{
					if ( event->y<InfosGene->BlockHeight*RUNG_HEIGHT-InfosGene->OffsetHiddenTopRungDisplayed )
						DoSelection = FALSE;
				}
				if ( DoSelection )
				{
					int NbrRungsShift =  (event->y+InfosGene->OffsetHiddenTopRungDisplayed)/(InfosGene->BlockHeight*RUNG_HEIGHT);
//printf("Select the current rung, with a shift of rungs=%d\n", NbrRungsShift );
					ChoiceOfTheCurrentRung( NbrRungsShift );
					MessageInStatusBar( GetLadderElePropertiesForStatusBar( event->x,event->y ) );
				}
			}
		}
	}
	return TRUE;
}

/* Draw a rectangle on the screen */
/*static void draw_brush( GtkWidget *widget,
                        gdouble    x,
                        gdouble    y)
{
    GdkRectangle update_rect;

    update_rect.x = x - 5;
    update_rect.y = y - 5;
    update_rect.width = 100;
    update_rect.height = 100;
    gdk_draw_rectangle (pixmap,
                        widget->style->black_gc,
                        TRUE,
                        update_rect.x, update_rect.y,
                        update_rect.width, update_rect.height);
    gtk_widget_draw (widget, &update_rect);
}*/

void refresh_label_comment( void )
{
	StrRung * RfhRung;
	if ( SectionArray[ InfosGene->CurrentSection ].Language==SECTION_IN_LADDER )
	{
		RfhRung = &RungArray[InfosGene->CurrentRung];
		gtk_entry_set_text((GtkEntry *)entrylabel,RfhRung->Label);
		gtk_entry_set_text((GtkEntry *)entrycomment,RfhRung->Comment);
	}
	else
	{
		gtk_entry_set_text((GtkEntry *)entrylabel,"");
		gtk_entry_set_text((GtkEntry *)entrycomment,"");
	}
}
void clear_label_comment()
{
	gtk_entry_set_text((GtkEntry *)entrylabel,"");
	gtk_entry_set_text((GtkEntry *)entrycomment,"");
}

void save_label_comment_edited()
{
	strcpy(EditDatas.Rung.Label,gtk_entry_get_text((GtkEntry *)entrylabel));
	strcpy(EditDatas.Rung.Comment,gtk_entry_get_text((GtkEntry *)entrycomment));
}

void autorize_prevnext_buttons(int Yes)
{
	if (Yes)
	{
		gtk_widget_set_sensitive(VScrollBar, TRUE);
		gtk_widget_set_sensitive(entrylabel, FALSE);
		gtk_widget_set_sensitive(entrycomment, FALSE);
	}
	else
	{
		gtk_widget_set_sensitive(VScrollBar, FALSE);
		gtk_widget_set_sensitive(entrylabel, TRUE);
		gtk_widget_set_sensitive(entrycomment, TRUE);
	}
}

void ButtonRunStop_click()
{ rtapi_print("run button pushed %d\n",InfosGene->LadderState);
	if (InfosGene->LadderState==STATE_RUN)
	{
		InfosGene->LadderState = STATE_STOP;
		gtk_label_set_text(GTK_LABEL(GTK_BIN(ButtonRunStop)->child),"Run");
	}
	else
	{
		InfosGene->LadderState = STATE_RUN;
		gtk_label_set_text(GTK_LABEL(GTK_BIN(ButtonRunStop)->child),"Stop");
	}
}

void CheckDispSymbols_toggled( )
{
	InfosGene->DisplaySymbols = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( CheckDispSymbols ) );
}


void StoreDirectorySelected(
	#ifndef GTK2
GtkFileSelection *selector, 
	#else
GtkFileChooser *selector,	
#endif
char cForLoadingProject)
{
    char * TempDir;
	
    TempDir = 
	#ifndef GTK2
	gtk_file_selection_get_filename (GTK_FILE_SELECTION(FileSelector));
	#else
	gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(FileSelector));
	#endif
    if ( cForLoadingProject )
        VerifyDirectorySelected( TempDir );
    else
        strcpy( CurrentProjectFileName, TempDir );
}


void LoadNewLadder()
{
	char ProjectLoadedOk;
    StoreDirectorySelected(
	#ifndef GTK2
	GTK_FILE_SELECTION(FileSelector)
	#else
	GTK_FILE_CHOOSER(FileSelector)
	#endif
	
, TRUE/*cForLoadingProject*/);
	
    if (InfosGene->LadderState==STATE_RUN)
        ButtonRunStop_click();
    InfosGene->LadderState = STATE_LOADING;
	ProjectLoadedOk = LoadProjectFiles( CurrentProjectFileName );
	if ( !ProjectLoadedOk )
		ShowMessageBox( "Load Error", "Failed to load the project file...", "Ok" );

	UpdateAllGtkWindows( );
	MessageInStatusBar( ProjectLoadedOk?"Project loaded (stopped).":"Project failed to load...");
#ifndef RT_SUPPORT
        OpenHardware( 0 );
//        ConfigHardware( );
	InfosGene->AskToConfHard = TRUE;
#endif
    InfosGene->LadderState = STATE_STOP;
}
void ButtonSave_click()
{
	if ( !SaveProjectFiles( CurrentProjectFileName ) )
		ShowMessageBox( "Save Error", "Failed to save the project file...", "Ok" );
}

void SaveAsLadder(void)
{
    StoreDirectorySelected(
	#ifndef GTK2
	GTK_FILE_SELECTION(FileSelector)
	#else
	GTK_FILE_CHOOSER(FileSelector)
	#endif
	
	, FALSE/*cForLoadingProject*/);
	if ( !SaveProjectFiles( CurrentProjectFileName ) )
		ShowMessageBox( "Save Error", "Failed to save the project file...", "Ok" );
}

#ifdef GTK2
void
on_filechooserdialog_save_response(GtkDialog  *dialog,gint response_id,gpointer user_data)
{
	printf("SAVE %s %d\n",gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(FileSelector)),response_id);	

	if(response_id==GTK_RESPONSE_ACCEPT || response_id==GTK_RESPONSE_OK)
		SaveAsLadder();
	gtk_widget_destroy(GTK_WIDGET(dialog));
}
void
on_filechooserdialog_load_response(GtkDialog  *dialog,gint response_id,gpointer user_data)
{
	printf("LOAD %s %d\n",gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(FileSelector)),response_id);	

	if(response_id==GTK_RESPONSE_ACCEPT || response_id==GTK_RESPONSE_OK)
		LoadNewLadder();
	gtk_widget_destroy(GTK_WIDGET(dialog));
}
#endif


void CreateFileSelection(char * Prompt,int Save)
{
    /* From the example in gtkfileselection help */
    /* Create the selector */
	#ifndef GTK2
	FileSelector = gtk_file_selection_new(Prompt);
	if (Save)
        gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION(FileSelector)->ok_button),
                            "clicked", GTK_SIGNAL_FUNC (SaveAsLadder), NULL);
    else
        gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION(FileSelector)->ok_button),
                           "clicked", GTK_SIGNAL_FUNC (LoadNewLadder), NULL);
    /* Ensure that the dialog box is destroyed when the user clicks a button. */
    gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(FileSelector)->ok_button),
                                           "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
                                           (gpointer) FileSelector);
    gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(FileSelector)->cancel_button),
                                           "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
                                           (gpointer) FileSelector);
	#else
	GtkFileFilter *FilterOldProjects, *FilterProjects;
	if(Save)
	{
		FileSelector = gtk_file_chooser_dialog_new (Prompt, NULL, GTK_FILE_CHOOSER_ACTION_SAVE,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	}
	else
	{
		FileSelector = gtk_file_chooser_dialog_new (Prompt, NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	}
	gtk_window_set_type_hint (GTK_WINDOW (FileSelector), GDK_WINDOW_TYPE_HINT_DIALOG);

	FilterOldProjects = gtk_file_filter_new( );
	gtk_file_filter_set_name( FilterOldProjects, "Old directories projects" );
	gtk_file_filter_add_pattern( FilterOldProjects, "*.csv" ); // old dir projects
	gtk_file_chooser_add_filter( GTK_FILE_CHOOSER(FileSelector), FilterOldProjects );
	FilterProjects = gtk_file_filter_new( );
	gtk_file_filter_set_name( FilterProjects, "ClassicLadder projects" );
	gtk_file_filter_add_pattern( FilterProjects, "*.clp" );
	gtk_file_chooser_add_filter( GTK_FILE_CHOOSER(FileSelector), FilterProjects );
	gtk_file_chooser_set_filter( GTK_FILE_CHOOSER(FileSelector), FilterProjects );

	gtk_window_set_modal(GTK_WINDOW(FileSelector), TRUE );

/*
  g_signal_connect ((gpointer) filechooserdialog, "file_activated",
                    G_CALLBACK (on_filechooserdialog_file_activated),
                    NULL);
					*/

	if(Save)
		g_signal_connect ((gpointer) FileSelector, "response",
                    G_CALLBACK (on_filechooserdialog_save_response),
                    NULL);
	else
		g_signal_connect ((gpointer) FileSelector, "response",
                    G_CALLBACK (on_filechooserdialog_load_response),
                    NULL);

	g_signal_connect_swapped ((gpointer) FileSelector, "close",
                            G_CALLBACK (gtk_widget_destroy),
                            GTK_OBJECT (FileSelector));
	#endif

	/* Display that dialog */
	gtk_widget_show (FileSelector);
}

void DoNewProject( void )
{
	ClassicLadder_InitAllDatas( );
	UpdateAllGtkWindows( );
}

void ButtonNew_click()
{
	ShowConfirmationBox("New","Do you really want to clear all datas ?",DoNewProject);
}
void ButtonLoad_click()
{
	CreateFileSelection("Please select the project to load",FALSE);
}

void ButtonSaveAs_click()
{
	CreateFileSelection("Please select the project to save",TRUE);
}

void ButtonReset_click()
{
//////	int StateBefore = InfosGene->LadderState;
//////	InfosGene->LadderState = STATE_STOP;
//////	// wait, to be sure calcs have ended...
//////	usleep( 100000 );
	StopRunIfRunning( );

	InitVars();
	PrepareAllDatasBeforeRun( );

//////	if ( StateBefore==STATE_RUN )
//////		InfosGene->LadderState = STATE_RUN;
	RunBackIfStopped( );
}

void ButtonConfig_click()
{
    OpenConfigWindowGtk( );
}

void ButtonAbout_click()
{
	/* From the example in gtkdialog help */
	GtkWidget *dialog, *label, *okay_button;
	/* Create the widgets */
	dialog = gtk_dialog_new();
	label = gtk_label_new ( CL_PRODUCT_NAME " v" CL_RELEASE_VER_STRING "\n" CL_RELEASE_DATE_STRING "\n"
						"As adapted for EMC-2\n"
						"Copyright (C) 2001-2007 Marc Le Douarain\nmarc . le - douarain /At\\ laposte \\DoT/ net\n"
						"http://www.sourceforge.net/projects/classicladder\n"
						"http://membres.lycos.fr/mavati/classicladder\n"
						"Released under the terms of the\nGNU Lesser General Public License v2.1");
	gtk_label_set_justify( GTK_LABEL(label), GTK_JUSTIFY_CENTER );
	okay_button = gtk_button_new_with_label("Okay");
	/* Ensure that the dialog box is destroyed when the user clicks ok. */
	gtk_signal_connect_object (GTK_OBJECT (okay_button), "clicked",
							GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->action_area),
					okay_button);
	gtk_widget_grab_focus(okay_button);
	/* Add the label, and show everything we've added to the dialog. */
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
					label);
	gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
	gtk_window_set_position(GTK_WINDOW(dialog),GTK_WIN_POS_CENTER);
	gtk_widget_show_all (dialog);
}

void ShowMessageBox(char * title,char * text,char * button)
{
	/* From the example in gtkdialog help */
	GtkWidget *dialog, *label, *okay_button;
	/* Create the widgets */
	dialog = gtk_dialog_new();
	label = gtk_label_new (text);
	okay_button = gtk_button_new_with_label(button);
	/* Ensure that the dialog box is destroyed when the user clicks ok. */
	gtk_signal_connect_object (GTK_OBJECT (okay_button), "clicked",
							GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->action_area),
					okay_button);
	gtk_widget_grab_focus(okay_button);
	/* Add the label, and show everything we've added to the dialog. */
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
					label);
	gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
	gtk_window_set_title(GTK_WINDOW(dialog),title);
	gtk_window_set_position(GTK_WINDOW(dialog),GTK_WIN_POS_CENTER);
	gtk_widget_show_all (dialog);
}

void DoFunctionOfConfirmationBox(void * (*function_to_do)(void *))
{
	gtk_widget_destroy(ConfirmDialog);
	(function_to_do)(NULL);
}
void ShowConfirmationBox(char * title,char * text,void * function_if_yes)
{
	/* From the example in gtkdialog help */
	GtkWidget *label, *yes_button, *no_button;
	/* Create the widgets */
	ConfirmDialog = gtk_dialog_new();
	label = gtk_label_new (text);
	yes_button = gtk_button_new_with_label("Yes");
	no_button = gtk_button_new_with_label("No");
	/* Ensure that the dialog box is destroyed when the user clicks ok. */
	gtk_signal_connect_object (GTK_OBJECT (no_button), "clicked",
							GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT(ConfirmDialog));
	gtk_signal_connect_object (GTK_OBJECT (yes_button), "clicked",
							GTK_SIGNAL_FUNC (DoFunctionOfConfirmationBox), function_if_yes);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(ConfirmDialog)->action_area),
					yes_button);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(ConfirmDialog)->action_area),
					no_button);
	gtk_widget_grab_focus(no_button);
	/* Add the label, and show everything we've added to the dialog. */
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(ConfirmDialog)->vbox),
					label);
	gtk_window_set_modal(GTK_WINDOW(ConfirmDialog),TRUE);
	gtk_window_set_title(GTK_WINDOW(ConfirmDialog),title);
	gtk_window_set_position(GTK_WINDOW(ConfirmDialog),GTK_WIN_POS_CENTER);
	gtk_widget_show_all (ConfirmDialog);
}


void QuitAppliGtk()
{
	ClassicLadderEndOfAppli( );
	gtk_exit(0);
}

void DoQuit( void )
{
	gtk_widget_destroy( RungWindow ); //sends signal "destroy" that will call QuitAppliGtk()...
}
void ConfirmQuit( void )
{
	if ( InfosGene->AskConfirmationToQuit )
		ShowConfirmationBox( "Sure?", "Do you really want to quit ?\nIf not saved, all modifications will be lost  \n", DoQuit );
	else
		DoQuit( );
}
gint RungWindowDeleteEvent( GtkWidget * widget, GdkEvent * event, gpointer data )
{
	ConfirmQuit( );
	// we do not want that the window be destroyed.
	return TRUE;
}

void MessageInStatusBar( char * msg )
{
  gtk_statusbar_pop(GTK_STATUSBAR(StatusBar), StatusBarContextId);
  gtk_statusbar_push(GTK_STATUSBAR(StatusBar), StatusBarContextId, msg);
}

void RungWindowInitGtk()
{
	GtkWidget *vbox,*hboxtop,*hboxbottom,*hboxbottom2;
	GtkWidget *hboxmiddle;
	GtkWidget *ButtonQuit;
	GtkWidget *ButtonNew,*ButtonLoad,*ButtonSave,*ButtonSaveAs,*ButtonReset,*ButtonConfig,*ButtonAbout;
	GtkWidget *ButtonEdit,*ButtonSymbols,*ButtonSpyVars;
#ifdef GNOME_PRINT_USE
	GtkWidget *ButtonPrint,*ButtonPrintPreview;
#endif
	GtkTooltips * TooltipsEntryLabel, * TooltipsEntryComment;

	RungWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title ((GtkWindow *)RungWindow, "Section Display");

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (RungWindow), vbox);
	gtk_widget_show (vbox);

	gtk_signal_connect (GTK_OBJECT (RungWindow), "destroy",
						GTK_SIGNAL_FUNC (QuitAppliGtk), NULL);

	hboxtop = gtk_hbox_new (FALSE,0);
	gtk_container_add (GTK_CONTAINER (vbox), hboxtop);
	gtk_widget_show(hboxtop);
	gtk_box_set_child_packing(GTK_BOX(vbox), hboxtop,
		/*expand*/ FALSE, /*fill*/ FALSE, /*pad*/ 0, GTK_PACK_START);

	TooltipsEntryLabel = gtk_tooltips_new();
	entrylabel = gtk_entry_new();
	gtk_widget_set_usize((GtkWidget *)entrylabel,80,0);
	gtk_entry_set_max_length((GtkEntry *)entrylabel,LGT_LABEL-1);
	gtk_entry_prepend_text((GtkEntry *)entrylabel,"");
	gtk_box_pack_start (GTK_BOX (hboxtop), entrylabel, FALSE, FALSE, 0);
	gtk_tooltips_set_tip ( TooltipsEntryLabel, entrylabel, "Label of the current selected rung", NULL );
	gtk_widget_show(entrylabel);
	TooltipsEntryComment = gtk_tooltips_new();
	entrycomment = gtk_entry_new();
	gtk_entry_set_max_length((GtkEntry *)entrycomment,LGT_COMMENT-1);
	gtk_entry_prepend_text((GtkEntry *)entrycomment,"");
	gtk_box_pack_start (GTK_BOX (hboxtop), entrycomment, TRUE, TRUE, 0);
	gtk_tooltips_set_tip ( TooltipsEntryComment, entrycomment, "Comment of the current selected rung", NULL );
	gtk_widget_show(entrycomment);

	CheckDispSymbols = gtk_check_button_new_with_label("Display symbols");
	gtk_box_pack_start( GTK_BOX (hboxtop), CheckDispSymbols, FALSE, FALSE, 0 );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( CheckDispSymbols ), InfosGene->DisplaySymbols );
	gtk_signal_connect( GTK_OBJECT(CheckDispSymbols), "toggled",
				(GtkSignalFunc)CheckDispSymbols_toggled, NULL );
	gtk_widget_show( CheckDispSymbols );

#if defined( RT_SUPPORT ) || defined( __XENO__ )
	DurationOfLastScan = gtk_entry_new();
	gtk_widget_set_usize((GtkWidget *)DurationOfLastScan,60,0);
//    gtk_entry_set_max_length((GtkEntry *)DurationOfLastScan,LGT_COMMENT-1);
//    gtk_entry_set_max_length((GtkEntry *)DurationOfLastScan,20);
	gtk_entry_prepend_text((GtkEntry *)DurationOfLastScan,"");
	gtk_box_pack_start (GTK_BOX (hboxtop), DurationOfLastScan, FALSE, FALSE, 0);
	gtk_widget_set_sensitive(DurationOfLastScan, FALSE);
	gtk_widget_show(DurationOfLastScan);
#endif


	hboxmiddle = gtk_hbox_new (FALSE,0);
	gtk_container_add (GTK_CONTAINER (vbox), hboxmiddle);
	gtk_widget_show(hboxmiddle);
	gtk_box_set_child_packing(GTK_BOX(vbox), hboxmiddle,
		/*expand*/ TRUE, /*fill*/ TRUE, /*pad*/ 0, GTK_PACK_START);

	/* Create the drawing area */
	drawing_area = gtk_drawing_area_new ();
	gtk_drawing_area_size (GTK_DRAWING_AREA (drawing_area) ,
							BLOCK_WIDTH_DEF*RUNG_WIDTH+OFFSET_X+5 ,
							BLOCK_HEIGHT_DEF*RUNG_HEIGHT+OFFSET_Y+30);
	gtk_box_pack_start (GTK_BOX (hboxmiddle), drawing_area, TRUE, TRUE, 0);
	gtk_widget_show (drawing_area);

	AdjustVScrollBar = (GtkAdjustment *)gtk_adjustment_new( 0, 0, 0, 0, 0, 0);
	VScrollBar = gtk_vscrollbar_new( AdjustVScrollBar );
	gtk_box_pack_start (GTK_BOX (hboxmiddle), VScrollBar, FALSE, FALSE, 0);
	gtk_widget_show (VScrollBar);

	AdjustHScrollBar = (GtkAdjustment *)gtk_adjustment_new( 0, 0, 0, 0, 0, 0);
	HScrollBar = gtk_hscrollbar_new( AdjustHScrollBar );
	gtk_box_pack_start (GTK_BOX (vbox), HScrollBar, FALSE, FALSE, 0);
	gtk_widget_show (HScrollBar);
	UpdateVScrollBar();

	gtk_signal_connect(GTK_OBJECT (AdjustVScrollBar), "value-changed",
						(GtkSignalFunc) VScrollBar_value_changed_event, 0);
	gtk_signal_connect(GTK_OBJECT (AdjustHScrollBar), "value-changed",
						(GtkSignalFunc) HScrollBar_value_changed_event, 0);

	/* Create the status bar */
    StatusBar = gtk_statusbar_new ();
//	gtk_statusbar_set_has_resize_grip( GTK_STATUSBAR(StatusBar), FALSE );
    gtk_box_pack_start (GTK_BOX(vbox), StatusBar, FALSE, FALSE, 0);
    gtk_widget_show (StatusBar);
    StatusBarContextId = gtk_statusbar_get_context_id( GTK_STATUSBAR(StatusBar), "Statusbar" );


	hboxbottom = gtk_hbox_new (FALSE,0);
	gtk_container_add (GTK_CONTAINER (vbox), hboxbottom);
	gtk_widget_show(hboxbottom);
	gtk_box_set_child_packing(GTK_BOX(vbox), hboxbottom,
		/*expand*/ FALSE, /*fill*/ FALSE, /*pad*/ 0, GTK_PACK_START);

	hboxbottom2 = gtk_hbox_new (FALSE,0);
	gtk_container_add (GTK_CONTAINER (vbox), hboxbottom2);
	gtk_widget_show(hboxbottom2);
	gtk_box_set_child_packing(GTK_BOX(vbox), hboxbottom2,
		/*expand*/ FALSE, /*fill*/ FALSE, /*pad*/ 0, GTK_PACK_START);

	ButtonNew = gtk_button_new_with_label ("New");
	gtk_box_pack_start (GTK_BOX (hboxbottom), ButtonNew, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT (ButtonNew), "clicked",
						(GtkSignalFunc) ButtonNew_click, 0);
	gtk_widget_show (ButtonNew);
	ButtonLoad = gtk_button_new_with_label ("Load");
	gtk_box_pack_start (GTK_BOX (hboxbottom), ButtonLoad, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT (ButtonLoad), "clicked",
						(GtkSignalFunc) ButtonLoad_click, 0);
	gtk_widget_show (ButtonLoad);
	ButtonSave = gtk_button_new_with_label ("Save");
	gtk_box_pack_start (GTK_BOX (hboxbottom), ButtonSave, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT (ButtonSave), "clicked",
						(GtkSignalFunc) ButtonSave_click, 0);
	gtk_widget_show (ButtonSave);
	ButtonSaveAs = gtk_button_new_with_label ("Save As");
	gtk_box_pack_start (GTK_BOX (hboxbottom), ButtonSaveAs, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT (ButtonSaveAs), "clicked",
						(GtkSignalFunc) ButtonSaveAs_click, 0);
	gtk_widget_show (ButtonSaveAs);
	ButtonReset = gtk_button_new_with_label ("Reset");
	gtk_box_pack_start (GTK_BOX (hboxbottom), ButtonReset, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT (ButtonReset), "clicked",
						(GtkSignalFunc) ButtonReset_click, 0);
	gtk_widget_show (ButtonReset);
	ButtonRunStop = gtk_button_new_with_label ("Stop");
	gtk_box_pack_start (GTK_BOX (hboxbottom), ButtonRunStop, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT (ButtonRunStop), "clicked",
						(GtkSignalFunc) ButtonRunStop_click, 0);
	gtk_widget_show (ButtonRunStop);
	ButtonSpyVars = gtk_button_new_with_label ("SpyVars");
	gtk_box_pack_start (GTK_BOX (hboxbottom), ButtonSpyVars, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT (ButtonSpyVars), "clicked",
						(GtkSignalFunc) OpenSpyVarsWindow, 0);
	gtk_widget_show (ButtonSpyVars);

	ButtonEdit = gtk_button_new_with_label ("Editor");
	gtk_box_pack_start (GTK_BOX (hboxbottom2), ButtonEdit, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT (ButtonEdit), "clicked",
						(GtkSignalFunc) OpenEditWindow, 0);
	gtk_widget_show (ButtonEdit);
	ButtonSymbols = gtk_button_new_with_label ("Symbols");
	gtk_box_pack_start (GTK_BOX (hboxbottom2), ButtonSymbols, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT (ButtonSymbols), "clicked",
						(GtkSignalFunc) OpenSymbolsWindow, 0);
	gtk_widget_show (ButtonSymbols);
	ButtonConfig = gtk_button_new_with_label ("Config");
	gtk_box_pack_start (GTK_BOX (hboxbottom2), ButtonConfig, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT (ButtonConfig), "clicked",
						(GtkSignalFunc) ButtonConfig_click, 0);
	gtk_widget_show (ButtonConfig);
#ifdef GNOME_PRINT_USE
	ButtonPrintPreview = gtk_button_new_with_label ("Preview");
	gtk_box_pack_start (GTK_BOX (hboxbottom2), ButtonPrintPreview, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT (ButtonPrintPreview), "clicked",
						(GtkSignalFunc) PrintPreviewGnome, 0);
	gtk_widget_show (ButtonPrintPreview);
	ButtonPrint = gtk_button_new_with_label ("Print");
	gtk_box_pack_start (GTK_BOX (hboxbottom2), ButtonPrint, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT (ButtonPrint), "clicked",
						(GtkSignalFunc) PrintGnome, 0);
	gtk_widget_show (ButtonPrint);
#endif
	ButtonAbout = gtk_button_new_with_label ("About");
	gtk_box_pack_start (GTK_BOX (hboxbottom2), ButtonAbout, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT (ButtonAbout), "clicked",
						(GtkSignalFunc) ButtonAbout_click, 0);
	gtk_widget_show (ButtonAbout);
	ButtonQuit = gtk_button_new_with_label ("Quit");
	gtk_box_pack_start (GTK_BOX (hboxbottom2), ButtonQuit, TRUE, TRUE, 0);
//    gtk_signal_connect_object (GTK_OBJECT (ButtonQuit), "clicked",
//                                GTK_SIGNAL_FUNC (gtk_widget_destroy),
//                                GTK_OBJECT (RungWindow));
	gtk_signal_connect_object (GTK_OBJECT (ButtonQuit), "clicked",
								ConfirmQuit, NULL);
	gtk_widget_show (ButtonQuit);


	/* Signals used to handle backing pixmap */
	gtk_signal_connect (GTK_OBJECT (drawing_area), "expose_event",
						(GtkSignalFunc) expose_event, NULL);
	gtk_signal_connect (GTK_OBJECT(drawing_area),"configure_event",
						(GtkSignalFunc) configure_event, NULL);

	/* Event signals */
	gtk_signal_connect (GTK_OBJECT (drawing_area), "button_press_event",
						(GtkSignalFunc) button_press_event, NULL);

	gtk_widget_set_events (drawing_area, GDK_EXPOSURE_MASK
							| GDK_LEAVE_NOTIFY_MASK
							| GDK_BUTTON_PRESS_MASK
							| GDK_POINTER_MOTION_MASK
							| GDK_POINTER_MOTION_HINT_MASK);

	gtk_signal_connect( GTK_OBJECT(RungWindow), "delete_event",
		(GtkSignalFunc)RungWindowDeleteEvent, 0 );
	gtk_widget_show (RungWindow);

	GetTheSizesForRung();
}

static gint PeriodicUpdateDisplay(gpointer data)
{
	if (InfosGene->LadderState==STATE_RUN)
	{
#if defined( RT_SUPPORT ) || defined( __XENO__ )
		char TextBuffer[ 20 ];
		sprintf(TextBuffer , "%d us", InfosGene->DurationOfLastScan/1000);
		gtk_entry_set_text(GTK_ENTRY(DurationOfLastScan),TextBuffer);
#endif
		if (InfosGene->CmdRefreshVarsBits)
		{
			RefreshAllBoolsVars();
			InfosGene->CmdRefreshVarsBits = FALSE;
		}
		DisplayFreeVarSpy();
	}
	if (InfosGene->LadderState!=STATE_LOADING )
		DrawCurrentSection( );
	if ( InfosGene->HardwareErrMsgToDisplay[ 0 ]!='\0' )
	{
		ShowMessageBox( "Config hardware error occured!", InfosGene->HardwareErrMsgToDisplay, "Ok" );
		InfosGene->HardwareErrMsgToDisplay[ 0 ] = '\0';
	}
	return 1;
}


void InitGtkWindows( int argc, char *argv[] )
{
	printf( "Your GTK+ version is %d.%d.%d\n", gtk_major_version, gtk_minor_version,
			gtk_micro_version );
//ProblemWithPrint	g_thread_init (NULL);
//ProblemWithPrint	gdk_threads_init ();
    gtk_init (&argc, &argv);

	VarsWindowInitGtk();
	RungWindowInitGtk();
//moved before, else crashing when adding tooltips...?
//	VarsWindowInitGtk();
	EditorInitGtk();
	PropertiesInitGtk();
	ManagerInitGtk( );
	SymbolsInitGtk( );

	gtk_timeout_add( TIME_UPDATE_GTK_DISPLAY_MS, PeriodicUpdateDisplay, NULL );
}

void UpdateAllGtkWindows( void )
{
	DrawCurrentSection( );
	refresh_label_comment( );
	autorize_prevnext_buttons( TRUE );
	UpdateVScrollBar();
	ManagerDisplaySections( );
	DisplaySymbols( );
}

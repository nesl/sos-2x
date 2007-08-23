#!/usr/bin/python
import sys, os
import gtk, gtk.glade
import pygtk
sys.path.append(os.environ['SOSROOT'] + '/modules/unit_test/python/')
import test_suite

class list_editor_gui():
    def __init__(self):
	self.gladefile = 'list_editor.glade'

        test_suite.configure_setup('../config.sys')

	self.new_list = []
	self.old_list = []
	self.curr_test = None

    def destroy(self, widget, data=None):
	gtk.main_quit()

    def draw_gui(self):
	self.gui = gtk.glade.XML(self.gladefile)
	self.gui.signal_autoconnect(self)
	self.w = self.gui.get_widget
	self.main_window = self.w('main_window')

	#set up old test list
	self.old_test_list_model = gtk.ListStore(int, str)
	self.old_test_list_view = self.w('old_test_list')
	self.old_test_list_view.set_model(self.old_test_list_model)

	add_column(self.old_test_list_view, 'Test #', 0, expand=False)
	add_column(self.old_test_list_view, 'Name', 1, expand=False)

	#set up new test list
	self.new_test_list_model = gtk.ListStore(int, str)
	self.new_test_list_view = self.w('new_test_list')
	self.new_test_list_view.set_model(self.new_test_list_model)

	add_column(self.new_test_list_view, 'Test #', 0, expand=False)
	add_column(self.new_test_list_view, 'Name', 1, expand=False)

    def save_new_test_list(self, button=None, filename=None):
	if not filename: filename = file_chooser_dialog(title="Save Test List", dlgtype="save", filters=(('test config files', '*.conf'), ('all files', '*')))
	if not filename: return

        new_test_f = open(filename, 'w')

	for test in self.new_list:
	    new_test_f.write(test.to_string())
	new_test_f.close()

    def open_test_list(self, button=None, filename=None):
	if not filename: filename = file_chooser_dialog(title="Open Test List", dlgtype="open", filters=(('test config files', '*.conf'), ("All Files", '*')))
	if not filename: return

        self.old_list = test_suite.configure_tests(filename)

        self.old_test_list_model.clear()
	i = 0
	for test in self.old_list:
	    self.old_test_list_model.append((i, test.name))
	    i+=1

    def clear_new_list(self, button=None, filename=None):
	self.new_list = []
	self.refresh_new_test_list()

    def refresh_new_test_list(self):
	self.new_test_list_model.clear()
	i = 0
	for test in self.new_list:
	    self.new_test_list_model.append((i, test.name))
	    i+=1

    def move_to_new_list(self, button=None):
	((index,), cursor) = self.old_test_list_view.get_cursor()

	self.new_list.append(self.old_list[index])
	self.refresh_new_test_list()

    def remove_from_new_list(self, button=None):
	((index,), cursor) = self.new_test_list_view.get_cursor()

	del self.new_list[index]
	self.refresh_new_test_list()

def file_chooser_dialog(title=None, dlgtype='open', filters=(('All files', '*'),)):

	if dlgtype == 'save':
		action = gtk.FILE_CHOOSER_ACTION_SAVE
		button = gtk.STOCK_SAVE
	else:
		action = gtk.FILE_CHOOSER_ACTION_OPEN
		button = gtk.STOCK_OPEN

	dlg = gtk.FileChooserDialog(
				title=title,
				action=action, 
				buttons=(gtk.STOCK_CANCEL,
					gtk.RESPONSE_CANCEL,
					button,
					gtk.RESPONSE_OK))

	dlg.set_default_response(gtk.RESPONSE_OK)
	dlg.set_do_overwrite_confirmation(True)

	for fdesc in filters:
		f = gtk.FileFilter()
		f.set_name(fdesc[0])
		f.add_pattern(fdesc[1])
		dlg.add_filter(f)

	response = dlg.run()
	filename = dlg.get_filename()
	dlg.destroy()

	if response == gtk.RESPONSE_OK: return filename 
	return None


def add_column(tree, title, column_id, sortable=True, cb=None, type=gtk.CellRendererText, no_text=False, expand=True, **kwargs):

	column = None
	kwargs['text'] = column_id

	if no_text: kwargs.pop('text')

	if cb:
		if cb[0] == 'edited':
			renderer = type()
			renderer.set_property('editable', True)
			renderer.connect('edited', cb[1], column_id)
			column = gtk.TreeViewColumn(title, renderer, **kwargs)
		else:
			renderer = type()
			renderer.connect(cb[0], cb[1], column_id)
			column = gtk.TreeViewColumn(title, renderer, **kwargs)
	else:
		renderer = type()
		column = gtk.TreeViewColumn(title, renderer, **kwargs)

	if sortable: column.set_sort_column_id(column_id)
	column.set_reorderable(True)
	column.set_resizable(True)		
	column.set_property('expand', expand)
	tree.append_column(column)

	return renderer

if __name__ == '__main__':
    app = list_editor_gui()
    app.draw_gui()

    gtk.main()

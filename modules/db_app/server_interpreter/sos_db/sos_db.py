import gtk, gtk.glade
import pygtk
from DB_Interface import DB_Interface
LESS_THAN = 1
GREATER_THAN = 2
LESS_EQUAL_THAN = 3
GREATER_EQUAL_THAN = 4
EQUAL = 5

AND = 1
OR = 2

class Qualifier():
    def __init__(self, sid, comp_op, comp_val, rel_op):
	self.sid = sid
	self.comp_op = comp_op
	self.rel_op = rel_op
	self.comp_val = comp_val

def convertStr(s):
    try:
	ret = int(s)
    except ValueError:
	ret = 0
    return ret

class DB_gui():
    def __init__(self):
	self.glade_file = 'sos_db.glade'

        self.avail_sensor = { 'x-accel':4, 'y-accel':5, 'mag-1':6, 'mag-2':7, 'photo':1, 'temp':2}
	self.sensor_query = []
	self.sensor_qualifier = []
	self.result_list = []
	self.qualifier = False 
	self.interface = DB_Interface()
	self.interface.setup_output(self.update_result_list)

    def destroy(self, widget, data=None):
	gtk.main_quit()

    def draw_gui(self):
	self.gui = gtk.glade.XML(self.glade_file)
	self.gui.signal_autoconnect(self)

	self.w = self.gui.get_widget

	self.main_window = self.w('main_window')
	self.main_window.connect('delete-event', gtk.main_quit)

        # set up the sensor list 
	self.sensor_list_model = gtk.ListStore(str)
	self.sensor_list_view = self.w('sensor_list')
	self.sensor_list_view.set_model(self.sensor_list_model)

	add_column(self.sensor_list_view, 'Sensor      ', 0, expand=False)
  
        for key in self.avail_sensor.keys():
	    print key
	    self.sensor_list_model.append([key])

        # set up the query list
	self.query_list_model = gtk.ListStore(str, str)
	self.query_list_view = self.w('query_list')
	self.query_list_view.set_model(self.query_list_model)

	add_column(self.query_list_view, 'Sensor', 0, expand=False)
	add_column(self.query_list_view, 'qual', 1, expand=False)

	#set up the results list
	self.results_list_model = gtk.ListStore(int, int, str, int)
	self.results_list_view = self.w('results_list')
	self.results_list_view.set_model(self.results_list_model)

	add_column(self.results_list_view, 'Qid', 0, expand=False)
	add_column(self.results_list_view, 'remaining', 1, expand=False)
	add_column(self.results_list_view, 'sensor', 2, expand=False)
	add_column(self.results_list_view, 'result', 3, expand=False)

	#set up the inputs for qualifiers
	self.sensor_name = self.w('sensor_name')
	self.comp_value = self.w('comp_value')
	self.qid = self.w('qid_entry')
	self.interval = self.w('interval_entry')
	self.n_samples = self.w('n_sample_entry')

	self.w('less_than_radio').connect('toggled', self.select_comp_op, LESS_THAN)
	self.w('greater_than_radio').connect('toggled', self.select_comp_op, GREATER_THAN)
	self.w('equal_radio').connect('toggled', self.select_comp_op, EQUAL)
	self.w('greater_than_equal_radio').connect('toggled', self.select_comp_op, GREATER_EQUAL_THAN)
	self.w('less_than_equal_radio').connect('toggled', self.select_comp_op, LESS_EQUAL_THAN)

        self.w('and_radio').connect('toggled', self.select_rel_op, AND)
	self.w('or_radio').connect('toggled', self.select_rel_op, OR)
	self.w('qualifier_check').connect('toggled', self.select_qualifier)

	self.current_comp_op = LESS_THAN
	self.current_rel_op = AND

    def select_qualifier(self, widget, data=None):
	self.qualifier = ~self.qualifier

    def select_comp_op(self, widget, data=None):
	self.current_comp_op = data

    def select_rel_op(self, widget, data=None):
	self.current_rel_op = data

    def create_new_query(self, button=None):
	if len(self.avail_sensor) == 0: return

	((index,), cursor) = self.sensor_list_view.get_cursor()
	iter = self.sensor_list_model.get_iter(index)
	(new_sensor,) = self.sensor_list_model.get(iter, 0)
	self.sensor_name.set_text(new_sensor)

    def add_new_query(self, button=None):
	self.sensor_query.append(self.avail_sensor[self.sensor_name.get_text()])
	if self.qualifier != 0:
	  print self.current_comp_op
	  print self.current_rel_op
	  print self.qualifier
	  self.sensor_qualifier.append( Qualifier(self.avail_sensor[self.sensor_name.get_text()], self.current_comp_op, 
	    self.comp_value.get_text(), self.current_rel_op))
	self.refresh_query_list()

    def refresh_query_list(self):
	for val in self.sensor_query:
	    self.query_list_model.append([val, 'no'])

    def submit_queries(self, button=None):
	qid= convertStr(self.qid.get_text())
	inter = convertStr(self.interval.get_text())
	n_samp = convertStr(self.n_samples.get_text())

	self.interface.insert_new_query(qid, inter, n_samp,[], self.sensor_query, self.sensor_qualifier)

    def update_result_list(self, qid, n_remain, results):
	for k, v in results.items():
	    self.result_list.append(qid, n_remain, s, v)
	    self.result_list_model.append([qid, n_remain, s,v])

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
    app = DB_gui()
    app.draw_gui()

    gtk.main()

	 

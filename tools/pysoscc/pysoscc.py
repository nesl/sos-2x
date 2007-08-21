#!/usr/bin/env python

import sys, os
import gtk, gtk.glade, pango
import pysos
import time as pytime
from datetime import *
from threading import Thread, Event
from include import *

try: import MySQLdb as sql
except: sql = None

class pysoscc(Thread):

	def __init__(self, parentlocals={}):

		self.nodelist = {}
		self.sossrv   = None
		self.sql      = None
		self.nodelist_refresh_timer  = None
		self.nodelist_timeout_timer  = None
		self.livetable_always_update = None 
		self.livetable_is_updating   = True 

		self.parentlocals = parentlocals

		# filter format is: did sid daddr saddr type desc 
		self.livefilters = []
		# trigger format is: did sid daddr saddr type func init_str
		self.triggers = []

		if 'uifile' not in dir(self):
			self.uifile = os.path.join(os.path.dirname(__file__), 'include', 'pysoscc.glade')

		Thread.__init__(self)


	def draw_gui(self):

		self.gui = gtk.glade.XML(self.uifile)
		self.gui.signal_autoconnect(self)
		self.w = self.gui.get_widget
		self.mainwindow = self.w('mainwindow')

		self.livetable_maxcount = self.w('live_buffer').get_value_as_int()

		# set up "node list" table
		self.nodelist_model = gtk.ListStore(int, str, str)
		self.nodelist_view = self.w('nodelist_table')
		self.nodelist_view.set_model(self.nodelist_model)

		add_column(self.nodelist_view, 'nid'      , 0, expand = False)
		add_column(self.nodelist_view, 'last seen', 1, expand = True)
		add_column(self.nodelist_view, 'status'   , 2, type = gtk.CellRendererPixbuf, no_text = True, stock_id = 2, expand = False)

		# set up "scripts" table
		self.scripts_model = gtk.ListStore(str, str, str) # image, name, location (hidden)
		self.scripts_view = self.w('scripts_table')
		self.scripts_view.set_model(self.scripts_model)
		self.scripts_view.connect('row-activated', self.execute_script)

		add_column(self.scripts_view, 'icon', 0, type = gtk.CellRendererPixbuf, no_text = True, stock_id = 0, expand = False)
		add_column(self.scripts_view, 'name', 1, expand = True)

		# set up "database" table
		self.dbtable_view = self.w('database_table')
		self.dbtable_model = gtk.ListStore(str)
		self.dbtable_view.set_model(self.dbtable_model)
		add_column(self.dbtable_view, 'Message', 0)
		self.dbtable_model.append(('No items to display',))

		# set up "live" table
		self.livetable_model = gtk.ListStore(str, str, int, int, int, int, int, int, str)
		self.livetable_view = self.w('live_table')
		self.livetable_view.set_model(self.livetable_model)

		add_column(self.livetable_view, 'time'       , 0, expand = False)
		add_column(self.livetable_view, 'description', 1, expand = False)
		add_column(self.livetable_view, 'did'        , 2, expand = False)
		add_column(self.livetable_view, 'sid'        , 3, expand = False)
		add_column(self.livetable_view, 'daddr'      , 4, expand = False)
		add_column(self.livetable_view, 'saddr'      , 5, expand = False)
		add_column(self.livetable_view, 'type'       , 6, expand = False)
		add_column(self.livetable_view, 'len'        , 7, expand = False)
		add_column(self.livetable_view, 'data'       , 8, expand = False)

		# set up "action triggers" table
		self.triggers_format = (int, int, int, int, int, str, str) 
		self.triggers_model = gtk.ListStore(*self.triggers_format)
		self.triggers_view = self.w('actiontriggers_table')
		self.triggers_view.set_model(self.triggers_model)

		add_column(self.triggers_view, 'did'           , 0, False, ('edited', self.edit_trigger), expand = False)
		add_column(self.triggers_view, 'sid'           , 1, False, ('edited', self.edit_trigger), expand = False)
		add_column(self.triggers_view, 'daddr'         , 2, False, ('edited', self.edit_trigger), expand = False)
		add_column(self.triggers_view, 'saddr'         , 3, False, ('edited', self.edit_trigger), expand = False)
		add_column(self.triggers_view, 'type'          , 4, False, ('edited', self.edit_trigger), expand = False)
		add_column(self.triggers_view, 'function'      , 5, False, ('edited', self.edit_trigger), expand = False)
		add_column(self.triggers_view, 'initialization', 6, False, ('edited', self.edit_trigger), expand = False)

		# set up "live filters" table
		self.livefilters_format = (int, int, int, int, int, str) 
		self.livefilters_model = gtk.ListStore(*self.livefilters_format)
		self.livefilters_view = self.w('livefilters_table')
		self.livefilters_view.set_model(self.livefilters_model)

		add_column(self.livefilters_view, 'did'        , 0, False, ('edited', self.edit_filter), expand = False)
		add_column(self.livefilters_view, 'sid'        , 1, False, ('edited', self.edit_filter), expand = False)
		add_column(self.livefilters_view, 'daddr'      , 2, False, ('edited', self.edit_filter), expand = False)
		add_column(self.livefilters_view, 'saddr'      , 3, False, ('edited', self.edit_filter), expand = False)
		add_column(self.livefilters_view, 'type'       , 4, False, ('edited', self.edit_filter), expand = False)
		add_column(self.livefilters_view, 'description', 5, False, ('edited', self.edit_filter), expand = False)

		# set up script properties dialog
		self.script_properties_dlg = self.w('script_properties_dlg')
		self.script_property_name  = self.w('script_property_name')
		self.script_property_file  = self.w('script_property_file')

		# set up embedded Python console
		self.pyshell = pyshell.Shell_Gui(with_window=0, localscope=self.parentlocals)
		self.w('console_wrapper').pack_start(self.pyshell.gui)

		# set up databse console
		self.dbconsole = self.w('database_console')
		self.dbconsole_clear()

		# load default options

		self.userdir = os.path.join(os.path.expanduser('~'), '.pysos')
		self.defaultworkspace = os.path.join(self.userdir, 'default.workspace')

		if not os.path.exists(self.userdir): 
			os.makedirs(self.userdir)

		self.new_workspace(ask=False)

		# other settings
		self.livetable_scroll        = self.w('live_scroll_chk').get_active()
		self.livetable_always_update = self.w('live_always_update').get_active()
		self.livetable_maxcount      = self.w('live_buffer').get_value_as_int()

		if not sql:
			self.w('database_tab').set_sensitive(False)
			self.w('database_label').set_sensitive(False)


	def run(self):

		self.draw_gui()
		gtk.gdk.threads_init()
		gtk.gdk.threads_enter()
		gtk.main()
		gtk.gdk.threads_leave()

	
	def about_show_dlg(self, *etc):

		self.w('about_dlg').show()

	
	def about_dlg_response(self, dlg, resp):
		if resp < 0: dlg.hide()
		dlg.emit_stop_by_name('response')


	def new_workspace(self, button=None, ask=True):

		if ask and not yesno_dialog('Are you sure you want to create a new workspace?', 'This will permanently erase your current settings!'): return

		if os.path.isfile(self.defaultworkspace):
			filename = self.defaultworkspace 
			self.open_workspace(filename=filename)

		else:
			self.new_scriptset  (ask=False)
			self.new_triggerset (ask=False)
			self.new_filterset  (ask=False)

			self.w('sossrv_host'        ).set_text('localhost')
			self.w('sossrv_port'        ).set_value(7915)
			self.w('live_buffer'        ).set_value(1000)
			self.w('live_scroll_chk'    ).set_active(True)
			self.w('live_always_update' ).set_active(False)
			self.w('nodelist_clear_chk' ).set_active(True)
			self.w('nodelist_timeout'   ).set_value(20)
			self.w('nodelist_refresh'   ).set_value(1)
			self.w('dbtable_user'       ).set_text('')
			self.w('dbtable_pass'       ).set_text('')
			self.w('dbtable_db'         ).set_text('')
			self.w('dbtable_host'       ).set_text('localhost:3306')

			self.save_workspace_as(self.defaultworkspace)


	def open_workspace(self, button=None, filename=None):

		if not filename: filename = file_chooser_dialog(title='Open workspace set', dlgtype='open', filters=(('Workspaces','*.workspace'),('All files', '*')))
		if not filename: return

		params = load_params_from_file(filename)[0]

		self.open_scriptset (params = params ['scripts'] )
		self.open_triggerset(params = params ['triggers'])
		self.open_filterset (params = params ['filters'] )

		self.w ( 'sossrv_host'        ).set_text   ( params ['sossrv_host'        ] )
		self.w ( 'sossrv_port'        ).set_value  ( params ['sossrv_port'        ] )
		self.w ( 'live_buffer'        ).set_value  ( params ['live_buffer'        ] )
		self.w ( 'live_scroll_chk'    ).set_active ( params ['live_scroll_chk'    ] )
		self.w ( 'live_always_update' ).set_active ( params ['live_always_update' ] )
		self.w ( 'nodelist_clear_chk' ).set_active ( params ['nodelist_clear_chk' ] )
		self.w ( 'nodelist_timeout'   ).set_value  ( params ['nodelist_timeout'   ] )
		self.w ( 'nodelist_refresh'   ).set_value  ( params ['nodelist_refresh'   ] )
		self.w ( 'dbtable_user'       ).set_text   ( params ['dbtable_user'       ] )
		self.w ( 'dbtable_pass'       ).set_text   ( params ['dbtable_pass'       ] )
		self.w ( 'dbtable_db'         ).set_text   ( params ['dbtable_db'         ] )
		self.w ( 'dbtable_host'       ).set_text   ( params ['dbtable_host'       ] )


	def save_workspace(self, *etc):

		filename = file_chooser_dialog(title='Save workspace set', dlgtype='save', filters=(('Workspaces','*.workspace'),) )
		if not filename: return

		self.save_workspace_as(filename)


	def save_default_workspace(self, *etc):

		self.save_workspace_as(self.defaultworkspace)


	def save_workspace_as(self, filename):
		params = {}
		
		params['scripts'  ] = self.save_scriptset  ( just_return = True )
		params['triggers' ] = self.save_triggerset ( just_return = True )
		params['filters'  ] = self.save_filterset  ( just_return = True )

		params['sossrv_host'        ] = self.w ( 'sossrv_host'        ).get_text()
		params['sossrv_port'        ] = self.w ( 'sossrv_port'        ).get_value()
		params['live_buffer'        ] = self.w ( 'live_buffer'        ).get_value()
		params['live_scroll_chk'    ] = self.w ( 'live_scroll_chk'    ).get_active()
		params['live_always_update' ] = self.w ( 'live_always_update' ).get_active()
		params['nodelist_clear_chk' ] = self.w ( 'nodelist_clear_chk' ).get_active()
		params['nodelist_timeout'   ] = self.w ( 'nodelist_timeout'   ).get_value()
		params['nodelist_refresh'   ] = self.w ( 'nodelist_refresh'   ).get_value()
		params['dbtable_user'       ] = self.w ( 'dbtable_user'       ).get_text()
		params['dbtable_pass'       ] = self.w ( 'dbtable_pass'       ).get_text()
		params['dbtable_db'         ] = self.w ( 'dbtable_db'         ).get_text()
		params['dbtable_host'       ] = self.w ( 'dbtable_host'       ).get_text()

		save_params_to_file(filename, params)


	def settings_show_menu(self, *etc):
		self.w('settingsmenu').popup(None, None, None, 1, 0)


	def db_connect(self, *etc):

		user = self.w('dbtable_user').get_text()
		passwd = self.w('dbtable_pass').get_text()
		db = self.w('dbtable_db').get_text()
		host = self.w('dbtable_host').get_text().split(':')
		port = 3306

		if len(host) > 1: port = int(host[1])
		host = host[0]

		try:
			if len(passwd) == 0:
				self.sql = sql.connect(user=user, host=host, port=port, db=db)
			else:
				self.sql = sql.connect(user=user, passwd=passwd, host=host, port=port, db=db)
		except: 
			self.show_error_dlg("Can't connect to database.")
			return

		self.cursor = self.sql.cursor(sql.cursors.SSCursor)
		self.w('dbtable_connect_btn').set_sensitive(False)
		self.w('dbtable_connect_btn').set_sensitive(False)
		self.w('dbtable_user').set_sensitive(False)
		self.w('dbtable_pass').set_sensitive(False)
		self.w('dbtable_db').set_sensitive(False)
		self.w('dbtable_host').set_sensitive(False)

		self.dbconsole_query('SHOW tables;')

	
	def db_disconnect(self, *etc):

		if self.sql: self.sql.close()
		self.sql = None
		self.w('dbtable_connect_btn').set_sensitive(True)
		self.w('dbtable_user').set_sensitive(True)
		self.w('dbtable_pass').set_sensitive(True)
		self.w('dbtable_db').set_sensitive(True)
		self.w('dbtable_host').set_sensitive(True)


	def dbconsole_execute(self, *etc):
		
		if not self.sql: 
			self.show_error_dlg("Not connected to database!")
			return

		buf = self.dbconsole.get_buffer()
		s = buf.get_start_iter()
		e = buf.get_end_iter()
		query = self.dbconsole.get_buffer().get_text(s, e)

		self.dbconsole_query(query)


	def dbconsole_query(self, query):

		try:
			count = self.cursor.execute(query)
		except: 
			self.show_error_dlg("Bad query.\n %s" % sys.exc_info()[0])
			return

		# clear database results table
		self.dbtable_model.clear()
		cols = self.dbtable_view.get_columns()

		for c in cols: self.dbtable_view.remove_column(c)

		if count == 0: 
			self.dbtable_model = gtk.ListStore(str)
			self.dbtable_view.set_model(self.dbtable_model)
			add_column(self.dbtable_view, 'Message', 0)
			self.dbtable_model.append(('No items to display',))
			return

		desc  = self.cursor.description
		row   = self.cursor.fetchone()
		L     = len(row)
		cols  = xrange(0,L)
		coltype = [type(r) for r in row]

		for i in cols: 
			if   coltype[i] == datetime : coltype[i] = str

		coltype.append(int)
		self.dbtable_model = gtk.ListStore(*coltype)

		row  = list(row) + [0]
		rows = (row,)
		j = 0

		while 1:
			for r in rows: 
				for i in cols: row[i] = coltype[i](r[i])
				row[L] = j
				j += 1
				try    : self.dbtable_model.append(row)
				except : pass
			# trying to save *some* memory
			rows = self.cursor.fetchmany(5000)
			if len(rows) == 0: break

		self.dbtable_view.set_model(self.dbtable_model)
		add_column(self.dbtable_view, '#', L)
		for i in cols: 
			title = desc[i][0].replace('_', ' ') 
			add_column(self.dbtable_view, title, i)


	def dbconsole_clear(self, *etc):
		self.dbconsole.get_buffer().set_text('SELECT * FROM tablename LIMIT 0,1000')


	def dbconsole_keypress(self, view, event):
		if event.keyval == gtk.keysyms.Return: 
			self.dbconsole_execute()
			return True

		
	def pyconsole_clear(self, *etc): 
		self.pyshell.clear_text()


	def pyconsole_reload(self, *etc): 
		
		if yesno_dialog('Are you sure you want to reload the console?'): 
			self.pyshell.reset_shell()


	def pyconsole_save(self, *etc):

		filename = file_chooser_dialog(title='Save commands as Python script', dlgtype='save', filters=(('Python files','*.py'),) )

		if not filename: return

		if os.path.isfile(filename):
			if not yesno_dialog('File already exists!', 'Do you want to overwrite it?'): 
				return

		try:
			file = open(filename,'w')
			for c in self.pyshell.history:
				file.write(c)
				file.write("\n")
			file.close()

		except Exception, x:
			alert_dialog('Error: could not write file.', x)




	def on_new_packet(self, msg):

		# filter packets
		matched = False
		for p in self.livefilters:
			if match_message(msg, p):
				matched = True
				break

		if not matched: return

		saddr = msg['saddr']

		# update node list
		if saddr in self.nodelist:
			iter = self.nodelist[saddr][0]
			self.nodelist_model.set_value(iter, 1, '0s ago')
			self.nodelist_model.set_value(iter, 2, gtk.STOCK_YES)
			self.nodelist[saddr][1] = pytime.time()
		else:
			self.nodelist[saddr] = [self.nodelist_model.append(), pytime.time()]
			iter = self.nodelist[saddr][0]
			self.nodelist_model.set_value(iter, 0, saddr)
			self.nodelist_model.set_value(iter, 1, '0s ago')
			self.nodelist_model.set_value(iter, 2, gtk.STOCK_YES)

		# if table length has reached maxcount, drop oldest row
		if len(self.livetable_model) >= self.livetable_maxcount:
			self.livetable_model.remove(self.livetable_model.get_iter_first())

		# add packet as new row
		self.livetable_view.freeze_child_notify()
		iter = self.livetable_model.append((pytime.strftime('%Y/%M/%d %H:%m:%S'), p[5], msg['did'], 
		                                    msg['sid'] , msg['daddr'] , msg['saddr'],
		                                    msg['type'], msg['length'], string2hex(msg['data'])))
		self.livetable_view.thaw_child_notify()

		# scroll to newest row
		if self.livetable_scroll and self.livetable_is_updating: 
			path = self.livetable_model.get_path(iter)
			self.livetable_view.scroll_to_cell(path)


	def add_filter(self, *args):

		desc = 'unnamed'
		if len(args) > 1: desc = args[1]

		iter = self.livefilters_model.append((0,0,0,0,0,desc))
		self.livefilters.append((0,0,0,0,0,desc))
		self.livefilters_view.get_selection().select_iter(iter)


	def remove_filter(self, *etc):

		selection = self.livefilters_view.get_selection()
		model, iter = selection.get_selected()
		if not iter: return

		row = model.get_path(iter)[0]
		model.remove(iter)
		self.livefilters.pop(row)

		if row >= len(self.livefilters): row -= 1
		if row <= 0: row = 0

		selection.select_path((row,))


	def add_trigger(self, *etc):

		iter = self.triggers_model.append((0,0,0,0,0,'',''))
		self.triggers.append((0,0,0,0,0,void,''))
		self.triggers_view.get_selection().select_iter(iter)


	def remove_trigger(self, *etc):

		selection = self.triggers_view.get_selection()
		model, iter = selection.get_selected()
		if not iter: return

		row = model.get_path(iter)[0]
		model.remove(iter)
		self.triggers.pop(row)

		if row >= len(self.triggers): row -= 1
		if row <= 0: row = 0

		selection.select_path((row,))


	def move_filter_up(self, *etc):
		
		selection = self.livefilters_view.get_selection()
		model, iter = selection.get_selected()
		if not iter: return
			
		row = model.get_path(iter)[0]
		model.remove(iter)
		f = self.livefilters.pop(row)

		if row > 0: row -= 1

		self.livefilters.insert(row, f)
		self.livefilters_model.insert(row, f)
		selection.select_path(row)


	def move_filter_down(self, *etc):

		selection = self.livefilters_view.get_selection()
		model, iter = selection.get_selected()
		if not iter: return

		row = model.get_path(iter)[0]
		model.remove(iter)
		f = self.livefilters.pop(row)

		if row < len(self.livefilters): row += 1

		self.livefilters.insert(row, f)
		self.livefilters_model.insert(row, f)
		selection.select_path(row)


	def move_trigger_up(self, *etc):
		
		selection = self.triggers_view.get_selection()
		model, iter = selection.get_selected()
		if not iter: return
			
		row = model.get_path(iter)[0]
		model.remove(iter)
		t = self.triggers.pop(row)

		if row > 0: row -= 1

		self.triggers.insert(row, t)
		self.triggers_model.insert(row, [self.triggers_format[i](t[i]) for i in xrange(0,len(t))])
		selection.select_path(row)


	def move_trigger_down(self, *etc):

		selection = self.triggers_view.get_selection()
		model, iter = selection.get_selected()
		if not iter: return

		row = model.get_path(iter)[0]
		model.remove(iter)
		t = self.triggers.pop(row)

		if row < len(self.triggers): row += 1

		self.triggers.insert(row, t)
		self.triggers_model.insert(row, [self.triggers_format[i](t[i]) for i in xrange(0,len(t))])
		selection.select_path(row)


	def edit_trigger(self, cell, rowstr, datastr, col):

		typed_data = self.triggers_format[col](datastr) 

		iter = self.triggers_model.get_iter_from_string(rowstr)
		wrapped_data = typed_data

		if col == 5: 
			if len(typed_data) == 0: wrapped_data = void
			elif typed_data in globals(): wrapped_data = globals()[typed_data]
			else: return self.show_error_dlg("Function '%s' does not exist!" % typed_data)

		row = int(rowstr)
 		f = list(self.triggers[row])
		f[col] = wrapped_data 
		self.triggers[row] = tuple(f)

		self.triggers_model.set_value(iter, col, typed_data)


	def edit_filter(self, cell, rowstr, datastr, col):

		row = int(rowstr)
		data = self.livefilters_format[col](datastr) 

		iter = self.livefilters_model.get_iter_from_string(rowstr)
		self.livefilters_model.set_value(iter, col, data)
		
 		f = list(self.livefilters[row])
		f[col] = data
		self.livefilters[row] = tuple(f)


	def update_nodelist_times(self):

		for n in self.nodelist.values():
			self.nodelist_model.set_value(n[0], 1, time_ellapsed(n[1]))

			if round(pytime.time() - n[1]) > self.nodelist_timeout: 
				self.nodelist_model.set_value(n[0], 2, gtk.STOCK_NO)


	def run_external_tool(self, dlg):

		if not self.livetable_always_update:
			self.livetable_view.set_sensitive(False)
			self.livetable_view.set_model(None)
			self.livetable_is_updating = False

		# TODO: run app here		

		if not self.livetable_always_update:
			self.livetable_is_updating = True 
			self.livetable_view.set_model(self.livetable_model)
			self.livetable_view.set_sensitive(True)

			if self.livetable_scroll and self.sossrv: 
				path = (self.livetable_model.iter_n_children(None)-1,)
				self.livetable_view.scroll_to_cell(path)


	def start_listening(self, *etc):

		self.livetable_model.clear()

		self.set_nodelist_options()
		self.livetable_scroll        = self.w('live_scroll_chk').get_active()
		self.livetable_always_update = self.w('live_always_update').get_active()
		self.livetable_maxcount      = self.w('live_buffer').get_value_as_int()

		host = self.w('sossrv_host').get_text()
		port = self.w('sossrv_port').get_value_as_int()

		try: 
			if self.sossrv: 
				self.sossrv.deregister_trigger()
				self.sossrv.reconnect(host=host, port=port)

			else: 
				self.sossrv = pysos.sossrv(host=host, port=port)

		except IOError:
			self.show_error_dlg("Failed to connect to %s:%d." % (host, port))
			return

		# register the trigger to display packets on the packet list
		self.sossrv.register_trigger(self.on_new_packet)

		# register all other triggers
		for t in self.triggers: 
			if len(t[6]): 
				try: eval(t[6])
				except:
					self.show_error_dlg('Error evaluating trigger initialization.')
					return

			self.sossrv.register_trigger(*t[:6])

		self.w('livetable_connect_btn').set_sensitive(False)
		self.w('sossrv_host').set_sensitive(False)
		self.w('sossrv_port').set_sensitive(False)
		self.w('displayoptions_pane').set_sensitive(False)


	def stop_listening(self, *etc):

		if self.nodelist_refresh_timer:
			self.nodelist_refresh_timer.stop()

		if self.sossrv: self.sossrv.disconnect()

		self.w('livetable_connect_btn').set_sensitive(True)
		self.w('sossrv_host').set_sensitive(True)
		self.w('sossrv_port').set_sensitive(True)
		self.w('displayoptions_pane').set_sensitive(True)


	def set_nodelist_options(self, *etc):
	
		self.nodelist_refresh = self.w('nodelist_refresh').get_value_as_int()
		self.nodelist_timeout = self.w('nodelist_timeout').get_value_as_int()

		if self.w('nodelist_clear_chk').get_active():
			self.nodelist = {} 
			self.nodelist_model.clear()

		if self.nodelist_refresh_timer: self.nodelist_refresh_timer.stop()
		self.nodelist_refresh_timer = Timer(self.nodelist_refresh, self.update_nodelist_times, periodic=True)
		self.nodelist_refresh_timer.start()

	def add_script(self, *etc):

		self.script_property_name.set_text('')
		self.script_property_file.set_filename('')
		self.script_property_file.grab_focus()
		reply = self.script_properties_dlg.run()
		self.script_properties_dlg.hide()

		if reply == 0: return

		filename = self.script_property_file.get_filename()
		name     = self.script_property_name.get_text()

		if not filename: return
		if not os.path.isfile(filename): return
		if not name: name = filename.split('/')[-1]

		selection   = self.scripts_view.get_selection()
		model, iter = selection.get_selected()

		newrow = (gtk.STOCK_FILE, name, filename)

		if not iter: 
			iter = self.scripts_model.append(newrow)
		else:
			iter = self.scripts_model.insert_after(iter, newrow)

		self.scripts_view.get_selection().select_iter(iter)


	def edit_script(self, *etc):

		selection   = self.scripts_view.get_selection()
		model, iter = selection.get_selected()

		if not iter: return

		name     = self.scripts_model.get_value(iter, 1)
		filename = self.scripts_model.get_value(iter, 2)

		if not name: name = filename.split('/')[-1]

		self.script_property_name.set_text(name)
		self.script_property_file.set_filename(filename)
		self.script_property_file.grab_focus()
		reply = self.script_properties_dlg.run()
		self.script_properties_dlg.hide()

		if reply == 0: return

		filename = self.script_property_file.get_filename()
		name     = self.script_property_name.get_text()

		self.scripts_model.set_value(iter, 1, name)
		self.scripts_model.set_value(iter, 2, filename)


	def execute_script(self, *etc):

		selection   = self.scripts_view.get_selection()
		model, iter = selection.get_selected()

		if not iter: return

		filepath = self.scripts_model.get_value(iter, 2)
		filetuple = filepath.split('/')
		directory = '/'.join(filetuple[:-1])
		filename = filetuple[-1]

		curr_directory = os.getcwd()
		os.chdir(directory)
		if 'startfile' in os.__dict__: os.startfile(filename)
		else: os.system(filepath)
		#execfile(filename, globals().copy())
		os.chdir(curr_directory)


	def move_script_up(self, *etc): 
		self.generic_move_up(self.scripts_view)

	
	def move_script_down(self, *etc): 
		self.generic_move_down(self.scripts_view)


	def remove_script(self, *etc):
		self.generic_remove(self.scripts_view)


	def generic_remove(self, view):

		selection   = view.get_selection()
		model, iter = selection.get_selected()
		if not iter: return

		row = model.get_path(iter)[0]
		model.remove(iter)

		if row >= model.iter_n_children(None): row -= 1
		if row < 0: row = 0

		selection.select_path((row,))


	def generic_move_up(self, view):

		selection   = view.get_selection()
		model, iter = selection.get_selected()

		if not iter: return

		path    = model.get_path(iter)
		newpath = (path[0]-1,)
		if newpath[0] < 0: return
		newiter = model.get_iter(newpath)

		model.swap(iter, newiter)
		selection.select_path(newpath)


	def generic_move_down(self, view):

		selection   = view.get_selection()
		model, iter = selection.get_selected()

		if not iter: return

		path    = model.get_path(iter)
		newpath = (path[0]+1,)
		if newpath[0] >= model.iter_n_children(None): return
		newiter = model.get_iter(newpath)

		model.swap(iter, newiter)
		selection.select_path(newpath)


	def new_scriptset(self, button=None, ask=True):
		
		if not ask or yesno_dialog('Are you sure you want to do this?', 'Your current script set will be permanently erased!'): 
			self.scripts_model.clear()


	def open_scriptset(self, button=None, filename=None, params=None):
	
		if params == None:

			if not filename: filename = file_chooser_dialog(title='Open script set', dlgtype='open', filters=(('Script sets','*.scripts'),('All files', '*')))
			if not filename: return

			scripts = load_params_from_file(filename)[0]

		else: scripts = params

		self.scripts_model.clear()
		for s in scripts: self.scripts_model.append(s)


	def save_scriptset(self, button=None, just_return=False):
	
		if not just_return:
			filename = file_chooser_dialog(title='Save script set', dlgtype='save', filters=(('Scripts sets','*.scripts'),) )
			if not filename: return

		model   = self.scripts_model
		iter    = model.get_iter_first()
		scripts = []

		while iter:
			scripts.append((model.get_value(iter, 0), model.get_value(iter, 1), model.get_value(iter, 2)))
			iter = self.scripts_model.iter_next(iter)

		if not just_return: save_params_to_file(filename, scripts, append=False)
		else: return scripts


	def save_filterset(self, button=None, just_return=False):
	
		if not just_return: 
			filename = file_chooser_dialog(title='Save filter set', dlgtype='save', filters=(('Filter sets','*.filters'),) )
			if not filename: return

		if not just_return: save_params_to_file(filename, self.livefilters, append=False)
		else: return self.livefilters 


	def open_filterset(self, button=None, filename=None, params=None):
	
		if params == None:

			if not filename: filename = file_chooser_dialog(title='Open filter set', dlgtype='open', filters=(('Filter sets','*.filters'),('All files', '*')))
			if not filename: return

			self.livefilters = load_params_from_file(filename)[0]

		else: self.livefilters = params

		self.livefilters_model.clear()

		for f in self.livefilters:
			self.livefilters_model.append(f)

	
	def new_filterset(self, button=None, ask=True):

		if ask and yesno_dialog('Are you sure you want to do this?', 'Your current filter set will be permanently erased!'): return

		self.livefilters_model.clear()
		self.livefitlers = []

		self.add_filter(0,'')


	def save_triggerset(self, button=None, just_return=None):
	
		if not just_return: 
			filename = file_chooser_dialog(title='Save trigger set', dlgtype='save', filters=(('Trigger sets','*.triggers'),) )
			if not filename: return

		triggers = []

		for t in self.triggers:

			triggers.append(list(t))
			triggers[-1][5] = t[5].func_name

			if triggers[-1][5] == 'void': triggers[-1][5] = ''

		if not just_return: save_params_to_file(filename, triggers, append=False)
		else: return triggers


	def open_triggerset(self, button=None, filename=None, params=None):
	
		if params == None:

			if not filename: filename = file_chooser_dialog(title='Open trigger set', dlgtype='open', filters=(('Trigger sets','*.triggers'),('All files', '*')))
			if not filename: return

			self.triggers = load_params_from_file(filename)[0]

		else: self.triggers = params

		self.triggers_model.clear()

		for i in xrange(len(self.triggers)):

			t = self.triggers[i]
			l = list(t)

			if len(t[5]) == 0: l[5] = void
			elif t[5] in globals(): l[5] = globals()[t[5]]
			else: return self.show_error_dlg("Function '%s' does not exist!" % t[5])

			self.triggers[i] = tuple(l)
			self.triggers_model.append(t)


	def new_triggerset(self, button=None, ask=True):

		if ask and yesno_dialog('Are you sure you want to do this?', 'Your current trigger set will be permanently erased!'): return
		self.triggers_model.clear()


	def livefilters_help(self, *etc):

		self.show_info_dlg("<big><b>Filters dictate which packets are shown in the 'Network Activity' \
table.</b></big>\n\nYou may enter a description for each packet type and use 0 as wild card.\n\nIf \
a packet matches more than one filter, <u>only the topmost</u> filter will be executed.")


	def triggers_help(self, *etc):

		self.show_info_dlg("<big><b>Action triggers are Python commands that are evaluated when \
a packet matches a certain pattern.</b></big>\n\nUse 0 as wild card, and enter the function as \
'<tt>myfunc</tt>' (without quotes). When called, functions are passed a message dict with fields: \
<tt>did</tt>, <tt>sid</tt>, <tt>daddr</tt>, <tt>saddr</tt>, <tt>type</tt>, <tt>len</tt>, and \
<tt>data</tt> (binary string).\n\nInitialization commands are Python commands that will be \
internally executed with the eval function.\n\nIf a packet matches more than one trigger, \
<u>all</u> triggers are executed, starting from the topmost.")


	def pyconsole_help(self, *etc):

		self.show_info_dlg("<big><b>Python Console for interacting with SOS.</b></big>\n\n\
This Console has a few nifty features, such as <b>history</b> (up/down keys) and \
<b>autocomplete</b> (CTRL+Space or Tab). Also, you may get help on something by \
typing a question mark after the object, like this '<tt>str?</tt>' or this \
'<tt>gtk.gdk?</tt>'.\n\n\
You may access this PySOSGUI instance on this console by using '<tt>app.&lt;something&gt;</tt>' \
For example, to communicate to the SOS Server connected to this application, use \
'<tt>app.sossrv</tt>' (without quotes). The database can be accessed with '<tt>app.db</tt>'.")


	def dbconsole_help(self, *etc):
		self.show_info_dlg("<big><b>Database Console</b></big>\n\n\
Use this console to enter SQL queries, such as '<tt>select * from mytable1 where sid = 0x20 \
limit 0,10</tt>', or '<tt>show tables</tt>'.\n\n\
To make binary data readable, you may want to tell SQL to print it as a hex string, like this: \
'<tt>select sid, hex(data) from mytable1</tt>'.")


	def scripts_help(self, *etc):
		self.show_info_dlg("<big><b>Application launcher</b></big>\n\n\
Use the application launcher to register program shortcuts!")

	
	def printstr(self, str):
		""" 
		Use this function to print to the embedded Python console.
		"""
		self.pyshell.redirectstd()
		sys.stdout.write('\n')
		sys.stdout.write(str)
		sys.stdout.write('\n')
		self.pyshell.restorestd()


	def show_info_dlg(self, info_string):

		error_dlg = gtk.MessageDialog(type=gtk.MESSAGE_INFO,
						message_format=info_string,
						buttons=gtk.BUTTONS_OK)

		error_dlg.set_property('use-markup', True)
		error_dlg.run()
		error_dlg.destroy()


	def show_error_dlg(self, error_string):

		error_dlg = gtk.MessageDialog(type=gtk.MESSAGE_ERROR,
						message_format=error_string,
						buttons=gtk.BUTTONS_OK)

		error_dlg.set_property('use-markup', True)
		error_dlg.run()
		error_dlg.destroy()


	def gtk_main_quit(self, *etc):
		self.stop_listening()
		gtk.main_quit()




# copied from threading module, but altered to allow periodic timers
class Timer(Thread):
	"""
	Call a function after a specified number of seconds:
	
	t = Timer(30.0, f, periodic=False, args=[], kwargs={})
	t.start()
	t.stop() # stop the timer's action if it's still waiting
	"""

	def __init__(self, interval, function, periodic=False, args=[], kwargs={}):

		Thread.__init__(self)
		self.interval = interval
		self.function = function
		self.args = args
		self.kwargs = kwargs
		self.finished = Event()
		self.oneshot = not periodic


	def stop(self):

		self.finished.set()


	def run(self):

		self.finished.clear()

		while not self.finished.isSet():

			self.finished.wait(self.interval)
			if not self.finished.isSet(): self.function(*self.args, **self.kwargs)
			if self.oneshot: self.finished.set()


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


def yesno_dialog(msg='Are you sure?', submsg=''):

	dlg = gtk.MessageDialog(parent=None, flags=gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT, buttons=gtk.BUTTONS_YES_NO)
	dlg.set_property('text', msg)
	dlg.set_property('secondary-text', submsg)
	response = dlg.run()
	dlg.destroy()
	return response == gtk.RESPONSE_YES


def alert_dialog(msg='', submsg=''):
	dlg = gtk.MessageDialog(None, gtk.DIALOG_MODAL, gtk.MESSAGE_ERROR, gtk.BUTTONS_CLOSE)
	dlg.set_markup('<big><b>%s</b></big>\n%s' % (msg, submsg))
	dlg.run()
	dlg.hide()


def load_params_from_file(filename):

	f      = file(filename, 'r')
	fstr   = f.readline()
	params = []

	while fstr:
		p = eval(fstr)
		params.append(p)
		fstr = f.readline()

	return params


def save_params_to_file(filename, params, append=False):

	mode = 'w'
	if append: mode = 'a'
	f = file(filename, mode)
	f.write(repr(params))
	f.close()


def string2hex(s):

	return ('%02x '*len(s)) % pysos.unpack('<'+len(s)*'B', s)


def time_ellapsed(t0):

	d = round(pytime.time() - t0)

	if d < 60     : return '%ds ago' % d
	elif d < 3600 : return '%dm %ds ago' % divmod(d,60)
	elif d < 86400: return '%dh %dm ago' % divmod(d,3600)
	else          : return '%dD %dh ago' % divmod(d,86400)

def void(*etc): pass
	
def match_message(msg, p):
	"""
	Returns True if the msg matches the pattern.
	"""
	return (((not p[0]) or (msg['did']   == p[0])) and 
	        ((not p[1]) or (msg['sid']   == p[1])) and 
	        ((not p[2]) or (msg['daddr'] == p[2])) and 
	        ((not p[3]) or (msg['saddr'] == p[3])) and 
	        ((not p[4]) or (msg['type']  == p[4])))


# if this file is being executed, run the main loop,
# otherwise, just load the defs and classes above and quit.
if __name__ == '__main__':
	app = pysoscc(parentlocals=locals())
	app.start()


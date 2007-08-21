# This file was edited from the Py_Shell.py found at 
# http://www.daa.com.au/pipermail/pygtk/2003-October/006046.html
# Original author: Pier Carteri
# Modified by: Thiago Teixeira

#
#       pyshell.py : inserts the python prompt in a gtk interface
#

import sys, code, os
import __builtin__


import gtk, gobject, pango

# these two must be same length
PS1 = ">>> "
PS2 = "... "

class Completer:
	"""
	Taken from rlcompleter, with readline references stripped, and a local dictionary to use.
	"""

	def __init__(self,locals):
		self.locals = locals


	def complete(self, text, state):
		"""
		Return the next possible completion for 'text'.
		This is called successively with state == 0, 1, 2, ... until it
		returns None.	The completion should begin with 'text'.
		"""

		if state == 0:
			if "." in text:
				self.matches = self.attr_matches(text)
			else:
				self.matches = self.global_matches(text)
		try:
			return self.matches[state]
		except IndexError:
			return None


	def global_matches(self, text):
		"""
		Compute matches when text is a simple name.

		Return a list of all keywords, built-in functions and names
		currently defined in __main__ that match.
		"""

		import keyword
		matches = []
		n = len(text)

		for list in [keyword.kwlist, __builtin__.__dict__.keys(), self.locals.keys()]:
			for word in list:
				if word[:n] == text and word != "__builtins__":
					matches.append(word)

		return matches


	def attr_matches(self, text):
		"""
		Compute matches when text contains a dot.

		Assuming the text is of the form NAME.NAME....[NAME], and is
		evaluatable in the globals of __main__, it will be evaluated
		and its attributes (as revealed by dir()) are used as possible
		completions. (For class instances, class members are are also
		considered.)

		WARNING: this can still invoke arbitrary C code, if an object
		with a __getattr__ hook is evaluated.
		"""

		import re

		m = re.match(r"(\w+(\.\w+)*)\.(\w*)", text)
		if not m: return

		expr, attr = m.group(1, 3)
		object = eval(expr, self.locals, self.locals)
		words = dir(object)

		if hasattr(object,'__class__'):
			words.append('__class__')
			words = words + get_class_members(object.__class__)

		matches = []
		n = len(attr)

		for word in words:
			if word[:n] == attr and word != "__builtins__":
				matches.append("%s.%s" % (expr, word))

		return matches


def get_class_members(klass):

	ret = dir(klass)

	if hasattr(klass,'__bases__'):
		 for base in klass.__bases__:
			 ret = ret + get_class_members(base)

	return ret


class Dummy_File:

	def __init__(self, buffer, tag):

		"""Implements a file-like object to redirect the stream to the buffer"""
		self.buffer = buffer
		self.tag = tag


	def write(self, text):

		"""Write text into the buffer and apply self.tag"""
		iter = self.buffer.get_end_iter()
		self.buffer.insert_with_tags(iter,text,self.tag)


	def writelines(self, l): map(self.write, l)
	def flush(self): pass
	def isatty(self): return 1



class PopUp:

	def __init__(self, text_view, token, list, position, n_chars):

		self.text_view = text_view
		self.token = token
		
		list.sort()
		
		self.list     = list
		self.position = position
		self.popup    = gtk.Window(gtk.WINDOW_POPUP)
		model = gtk.ListStore(gobject.TYPE_STRING)
		frame = gtk.Frame()
		sw    = gtk.ScrolledWindow()
		sw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)

		for item in self.list:
			iter = model.append()
			model.set(iter, 0, item)

		self.list_view = gtk.TreeView(model)
		self.list_view.connect("row-activated", self.hide)
		self.list_view.set_property("headers-visible", False)

		selection = self.list_view.get_selection()
		#selection.connect("changed",self.select_row)
		selection.select_path((0,))

		renderer = gtk.CellRendererText()
		column   = gtk.TreeViewColumn("",renderer,text = 0)

		self.list_view.append_column(column)
		sw.add(self.list_view)
		frame.add(sw)
		self.popup.add(frame)
		
		# set the width of the popup according with the length of the strings
		contest = self.popup.get_pango_context()
		desc    = contest.get_font_description()
		lang    = contest.get_language()
		metrics = contest.get_metrics(desc, lang)
		width   = pango.PIXELS(metrics.get_approximate_char_width()*n_chars)

		if width > 80 : self.popup.set_size_request(width+80, 90)
		else          : self.popup.set_size_request(160     , 90)

		self.show_popup()

 
	def hide(self, *arg):

		self.popup.hide()
		 

	def show_popup(self):

		buffer = self.text_view.get_buffer()
		iter   = buffer.get_iter_at_mark(buffer.get_insert())
		
		rectangle  = self.text_view.get_iter_location(iter)
		absX, absY = self.text_view.buffer_to_window_coords(gtk.TEXT_WINDOW_TEXT,
								   rectangle.x, #+rectangle.width+20 ,
								   rectangle.y+rectangle.height) #+20)

		parent = self.text_view.get_parent()
		self.popup.move(self.position[0]+absX, self.position[1]+absY)
		self.popup.show_all()


	def prev(self):

		sel         = self.list_view.get_selection()
		model, iter = sel.get_selected()
		newIter     = model.get_path(iter)

		if newIter != None and newIter[0]>0:
			path = (newIter[0]-1,)
			self.list_view.set_cursor(path)
			

	def next(self):

		sel         = self.list_view.get_selection()
		model, iter = sel.get_selected()
		newIter     = model.iter_next(iter)

		if newIter != None:
			path = model.get_path(newIter)
			self.list_view.set_cursor(path)


	def sel_confirmed(self):

		sel = self.list_view.get_selection()
		self.select_row(sel)
		self.hide()

																																				
	def sel_canceled(self):

		self.set_text(self.token)
		self.hide()


	def select_row(self, selection):

		model, iter = selection.get_selected()
		name = model.get_value(iter, 0)
		self.set_text(name)

	
	def set_text(self, text):

		buffer = self.text_view.get_buffer()
		end    = buffer.get_iter_at_mark(buffer.get_insert())
		start  = end.copy()

		start.backward_char()
		while start.get_char() not in "\t ,()[]=": start.backward_char()
		start.forward_char()

		buffer.delete(start, end)
		iter = buffer.get_iter_at_mark(buffer.get_insert())
		buffer.insert(iter, text)


class Shell_Gui:

	def __init__(self, with_window = 1, localscope = {}):
		
		sw = gtk.ScrolledWindow()
		sw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		sw.set_shadow_type(gtk.SHADOW_IN)
		t_table = gtk.TextTagTable()

		# create two tags: one for std_err and one for std_out

		tag_err = gtk.TextTag("error")
		tag_err.set_property("foreground","red")
		tag_err.set_property("font", "monospace")
		t_table.add(tag_err)
		
		tag_out = gtk.TextTag("output")
		tag_out.set_property("foreground","dark green")
		tag_out.set_property("font", "monospace")
		t_table.add(tag_out)
		
		tag_no_edit = gtk.TextTag("no_edit")
		tag_no_edit.set_property("editable", False)
		t_table.add(tag_no_edit)
		
		self.buffer = gtk.TextBuffer(t_table)

		self.buffer.set_text(PS1)
		start, end = self.buffer.get_bounds()
		self.buffer.apply_tag_by_name("output" , start, end)
		self.buffer.apply_tag_by_name("no_edit", start, end)

		self.view = gtk.TextView()
		self.view.set_buffer(self.buffer)
		self.view.connect("key_press_event", self.key_press)
		self.view.connect("drag_data_received",self.drag_data_received)
		self.view.set_wrap_mode(gtk.WRAP_CHAR)
		fontdesc = pango.FontDescription("monospace")
		self.view.modify_font(fontdesc)
		sw.add(self.view)

		# creates two dummy files
		self.dummy_err = Dummy_File(self.buffer,tag_err)
		self.dummy_out = Dummy_File(self.buffer,tag_out)
		
		# creates the console
		self.core = code.InteractiveConsole(locals = localscope)
		self.localscope = localscope

		# autocompletation capabilities
		self.completer = Completer(self.core.locals)
		self.popup = None
		
		# creates history capabilities
		self.history = [""]
		self.history_pos = 0

		if with_window:

			# add buttons
			b_box = gtk.Toolbar()
			b_box.set_orientation(gtk.ORIENTATION_HORIZONTAL)
			b_box.set_style(gtk.TOOLBAR_ICONS)
			b_box.insert_stock(gtk.STOCK_CLEAR,"Clear the output", None, self.clear_or_reset, None,-1)
			b_box.insert_stock(gtk.STOCK_SAVE,"Save the output", None, self.save_text, None,-1)
		
			box = gtk.VBox()
			box.pack_start(sw, expand = True, fill = True)
			box.pack_start(b_box, expand = False, fill = True)
#			frame = gtk.Frame(label_text)
#			frame.show_all()
#			frame.add(box)
		
			self.gui = gtk.Window()
			self.gui.add(box)
			#self.gui.add(sw)
			self.gui.connect("destroy",self.quit)
			self.gui.set_default_size(520,200)
			self.gui.show_all()

		else:
			self.gui = sw
			self.gui.show_all()
		




	def key_press(self, view, event):

		# if autocomplete popup is showing
		if self.popup != None:
			
			# key: up
			# action: next suggestion
			if event.keyval == gtk.keysyms.Up:

				self.popup.prev()
				return True 

			# key: down
			# action: previous suggestion
			elif event.keyval == gtk.keysyms.Down:

				self.popup.next()
				return True 

			# TODO: add pgup/pgdown/home/end

			# key: return
			# action: accept suggestion
			elif event.keyval == gtk.keysyms.Return:

				self.popup.sel_confirmed()
				self.popup = None
				return True 

			# key: escape
			# action: cancel autocomplete
			elif event.keyval == gtk.keysyms.Escape:

				self.popup.sel_canceled()
				self.popup = None
				return True 

			# key: any legal variable naming character
			# action: update suggestions
			elif ord('A') <= event.keyval <= ord('Z') or\
			     ord('a') <= event.keyval <= ord('z') or\
			     ord('0') <= event.keyval <= ord('9') or\
			     ord('_') == event.keyval:

				self.popup.hide()
				self.popup = None
				self.buffer.insert_at_cursor(unichr(event.keyval))
				self.complete_text(forcepopup=True)
				return True

			# key: anithing else
			# action: hide suggestions
			else:

				self.popup.hide()
				self.popup = None

				# printable characters
				if ord(' ') <= event.keyval <= ord('~'):
					self.buffer.insert_at_cursor(unichr(event.keyval))

				return True

		# if autocomplete popup is not being shown 
		else:

			# key: Up
			# action: move up or history
			if event.keyval == gtk.keysyms.Up:
				
#				last_line = self.buffer.get_end_iter().get_line()
#				cur_line  = self.buffer.get_iter_at_mark(self.buffer.get_insert()).get_line()
#				
#				if last_line == cur_line:

				if self.history_pos >= 0:
					# remove text into the line...
					end = self.buffer.get_end_iter()
					start = self.buffer.get_iter_at_line(end.get_line())
					start.forward_chars(len(PS1))
					self.buffer.delete(start,end)
					# insert the new text
					pos = self.buffer.get_end_iter()
					self.buffer.insert(pos, self.history[self.history_pos])
					self.history_pos -= 1
				else:
					gtk.gdk.beep()
					self.view.emit_stop_by_name("key-press-event")

				return True
				
			# key: Down
			# action: move down or history
			elif event.keyval == gtk.keysyms.Down:
	
#				last_line = self.buffer.get_end_iter().get_line()
#				cur_line  = self.buffer.get_iter_at_mark(self.buffer.get_insert()).get_line()
#				
#				if last_line == cur_line:

				if self.history_pos <= len(self.history)-1:
					# remove text into the line...
					end = self.buffer.get_end_iter()
					start = self.buffer.get_iter_at_line(end.get_line())
					start.forward_chars(len(PS1))
					self.buffer.delete(start,end)
					# insert the new text
					pos = self.buffer.get_end_iter()
					self.history_pos += 1
					self.buffer.insert(pos, self.history[self.history_pos])
					
				else:
					gtk.gdk.beep()
					self.view.emit_stop_by_name("key-press-event")
					
				return True
			
			# key: Tab
			# action: indent or autocomplete
			elif event.keyval == gtk.keysyms.Tab:

				iter = self.buffer.get_iter_at_mark(self.buffer.get_insert())

				iter.backward_char()
				just_add_tab = iter.get_char() in '\t\n '
				iter.forward_char()

				if just_add_tab: self.buffer.insert(iter, '\t')
				else: self.complete_text()

				return True
			
			# key: Shift+Tab
			# action: remove one level of indent
			elif event.keyval == gtk.keysyms.ISO_Left_Tab:

				start = self.buffer.get_iter_at_mark(self.buffer.get_insert())
				if start.get_line_offset() <= len(PS1): return True
				start.set_line_offset(len(PS1))
				end = start.copy()
				end.forward_char()
				self.buffer.delete(start, end)
				return True

			# key: Return
			# action: execute
			elif event.keyval == gtk.keysyms.Return:

				command = self.get_line()

				if len(command) > 0 and command[0] == '?':
					command = 'help(%s)' % command[1:]

				elif len(command) > 0 and command[-1] == '?':
					command = 'help(%s)' % command[:-1]

				self.exec_code(command)
				start,end = self.buffer.get_bounds()
				self.buffer.apply_tag_by_name("no_edit",start,end)
				self.buffer.place_cursor(end)
				return True
				
			# key: Ctrl+space
			# action: autocomplete
			elif event.keyval == gtk.keysyms.space and event.state & gtk.gdk.CONTROL_MASK:

				self.complete_text()
				return True
			
			# key: Home
			# action: go to beginning of line, but stop at '>>>'
			elif event.keyval == gtk.keysyms.Home and not (event.state & gtk.gdk.SHIFT_MASK):

				last_line = self.buffer.get_end_iter().get_line()
				cur_line  = self.buffer.get_iter_at_mark(self.buffer.get_insert()).get_line()

				if last_line == cur_line:
					iter = self.buffer.get_iter_at_line(cur_line)
					iter.forward_chars(4)
					self.buffer.place_cursor(iter)

				self.view.emit_stop_by_name("key-press-event")
				return True    

			# key: Left
			# action: move left, but stop at '>>>'
			elif event.keyval == gtk.keysyms.Left:

				last_line = self.buffer.get_end_iter().get_line()
				cur_pos   = self.buffer.get_iter_at_mark(self.buffer.get_insert())
				cur_line  = cur_pos.get_line()

				if last_line == cur_line:

					if cur_pos.get_line_offset() == 4:
						self.view.emit_stop_by_name("key-press-event")
						return True 	

#			# key: Ctrl + D
#			# action: restart the python console
#			elif (event.keyval == gtk.keysyms.d and event.state & gtk.gdk.CONTROL_MASK):
#
#				self.buffer.set_text(PS1)
#				start,end = self.buffer.get_bounds()
#				self.buffer.apply_tag_by_name("output",start,end)
#				self.buffer.apply_tag_by_name("no_edit",start,end)
#
#				# creates the console
#				self.core = code.InteractiveConsole(locals = self.localscope)
#
#				# reset history
#				self.history = [""]
#				self.history_pos = 0


	def clear_or_reset(self,*widget):

		dlg = gtk.Dialog("Clear")
		dlg.add_button("Clear",1)
		dlg.add_button("Reset",2)
		dlg.add_button(gtk.STOCK_CLOSE,gtk.RESPONSE_CLOSE)
		dlg.set_default_size(250,150)
		hbox = gtk.HBox()
		# add an image
		img = gtk.Image()
		img.set_from_stock(gtk.STOCK_CLEAR, gtk.ICON_SIZE_DIALOG)
		hbox.pack_start(img)
		
		# add text
		text = "You have two options:\n"
		text += "   - clear only the output window\n"
		text += "   - reset the shell\n"
		text += "\n What do you want to do?"
		label = gtk.Label(text)
		hbox.pack_start(label)
		
		hbox.show_all()
		dlg.vbox.pack_start(hbox)
		
		ans = dlg.run()
		dlg.hide()

		if   ans == 1 : self.clear_text()
		elif ans == 2 : self.reset_shell()


	def clear_text(self):

		self.buffer.set_text(PS1)
		start,end = self.buffer.get_bounds()
		self.buffer.apply_tag_by_name("output",start,end)
		self.buffer.apply_tag_by_name("no_edit",start,end)
		self.view.grab_focus()


	def reset_shell(self):

		self.buffer.set_text(PS1)
		start,end = self.buffer.get_bounds()
		self.buffer.apply_tag_by_name("output",start,end)
		self.buffer.apply_tag_by_name("no_edit",start,end)

		# creates the console
		self.core = code.InteractiveConsole(locals = self.localscope)

		# reset history
		self.history = [""]
		self.history_pos = 0


	def save_text(self, *widget):

		dlg = gtk.Dialog("Save to file")
		dlg.add_button("Commands",1)
		dlg.add_button("All",2)
		dlg.add_button(gtk.STOCK_CLOSE,gtk.RESPONSE_CLOSE)
		dlg.set_default_size(250,150)
		hbox = gtk.HBox()
		#add an image
		img = gtk.Image()
		img.set_from_stock(gtk.STOCK_SAVE, gtk.ICON_SIZE_DIALOG)
		hbox.pack_start(img)
		
		#add text
		text = "You have two options:\n"
		text += "   -save only commands\n"
		text += "   -save all\n"
		text += "\n What do you want to save?"
		label = gtk.Label(text)
		hbox.pack_start(label)
		
		hbox.show_all()
		dlg.vbox.pack_start(hbox)
		
		ans = dlg.run()
		dlg.hide()

		if ans == 1 :

			def ok_save(button, data = None):
				win =button.get_toplevel()
				win.hide()
				name = win.get_filename()
				if os.path.isfile(name):
					box = gtk.MessageDialog(dlg,
									  gtk.DIALOG_DESTROY_WITH_PARENT,
									  gtk.MESSAGE_QUESTION,gtk.BUTTONS_YES_NO,
									name+" already exists; do you want to overwrite it?"
									)
					ans = box.run()
					box.hide()
					if ans == gtk.RESPONSE_NO: return

				try:
					file = open(name,'w')
					for i in self.history:
						file.write(i)
						file.write("\n")
					file.close()
					
						
				except Exception, x:
					box = gtk.MessageDialog(dlg,
									  gtk.DIALOG_DESTROY_WITH_PARENT,
									  gtk.MESSAGE_ERROR,gtk.BUTTONS_CLOSE,
									"Unable to write \n"+
									name+"\n on disk \n\n%s"%(x)
									)
					box.run()
					box.hide()
					
			def cancel_button(button):
				win.get_toplevel()
				win.hide()
				
			win = gtk.FileSelection("Save Commands...")
			win.ok_button.connect_object("clicked", ok_save,win.ok_button)
			win.cancel_button.connect_object("clicked", cancel_button,win.cancel_button)
			win.show()

		elif ans == 2:

			def ok_save(button, data = None):
				win =button.get_toplevel()
				win.hide()
				name = win.get_filename()
				if os.path.isfile(name):
					box = gtk.MessageDialog(dlg,
									  gtk.DIALOG_DESTROY_WITH_PARENT,
									  gtk.MESSAGE_QUESTION,gtk.BUTTONS_YES_NO,
									name+" already exists; do you want to overwrite it?"
									)
					ans = box.run()
					box.hide()
					if ans == gtk.RESPONSE_NO:
						return
				try:
					start,end = self.buffer.get_bounds()
					text = self.buffer.get_text(start,end,0)
					file = open(name,'w')
					file.write(text)
					file.close()
					
				except Exception, x:
					box = gtk.MessageDialog(dlg,
									  gtk.DIALOG_DESTROY_WITH_PARENT,
									  gtk.MESSAGE_ERROR,gtk.BUTTONS_CLOSE,
									"Unable to write \n"+
									name+"\n on disk \n\n%s"%(x)
									)
					box.run()
					box.hide()
					
			def cancel_button(button):
				win.get_toplevel()
				win.hide()
				
			win = gtk.FileSelection("Save Log...")
			win.ok_button.connect_object("clicked", ok_save,win.ok_button)
			win.cancel_button.connect_object("clicked", cancel_button,win.cancel_button)
			win.show()

		dlg.destroy()
		self.view.grab_focus()
		

		  
	def get_line(self):

		iter = self.buffer.get_iter_at_mark(self.buffer.get_insert())
		line = iter.get_line()
		start = self.buffer.get_iter_at_line(line)
		end = start.copy()
		end.forward_line()
		command = self.buffer.get_text(start,end,0)
		if  (command[:4] == PS1 or command[:4] == PS2):
			command = command[4:]
		return command
		
		
	def complete_text(self, forcepopup=False):

		end   = self.buffer.get_iter_at_mark(self.buffer.get_insert())
		start = end.copy()

		start.backward_char()
		while start.get_char() not in "\t ,()[]=": start.backward_char()
		start.forward_char()

		token       = self.buffer.get_text(start,end,0).strip()
		completions = []

		try:
			p = self.completer.complete(token,len(completions))

			while p != None:
			  completions.append(p)
			  p = self.completer.complete(token, len(completions))

		except: return 

		# avoid duplicate items in 'completions'
		tmp = {}
		n_chars = 0

		for item in completions:
			dim = len(item)
			if dim>n_chars: n_chars = dim
			tmp[item] = None 

		completions = tmp.keys()

		if len(completions) > 1 or forcepopup:

			# show a popup 
			if isinstance(self.gui, gtk.ScrolledWindow):
				rect = self.gui.get_allocation()
				app = self.gui.window.get_root_origin()
				position = (app[0]+rect.x,app[1]+rect.y)
			else:	
				position = self.gui.window.get_root_origin()
		
			self.popup = PopUp(self.view, token, completions, position, n_chars) 

		elif len(completions) == 1:
			self.buffer.delete(start,end)
			iter = self.buffer.get_iter_at_mark(self.buffer.get_insert())
			self.buffer.insert(iter,completions[0])

		 
	def replace_line(self, text):

		iter = self.buffer.get_iter_at_mark(self.buffer.get_insert())
		line = iter.get_line()
		start = self.buffer.get_iter_at_line(line)
		start.forward_chars(4)
		end = start.copy()
		end.forward_line()
		self.buffer.delete(start,end)
		iter = self.buffer.get_iter_at_mark(self.buffer.get_insert())
		self.buffer.insert(iter, text)
						  

	def redirectstd(self):

		"""switch stdin stdout stderr to my dummy files"""
		self.std_out_saved = sys.stdout
		self.std_err_saved = sys.stderr
		
		sys.stdout = self.dummy_out
		sys.stderr = self.dummy_err


	def restorestd(self):

		"""switch my dummy files to stdin stdout stderr  """
		sys.stdout = self.std_out_saved
		sys.stderr = self.std_err_saved


	def drag_data_received(self, source, drag_context, n1, n2, selection_data, long1, long2):

		print selection_data.data

		
	def exec_code(self, text):
		"""Execute text into the console and display the output into TextView"""
		
		# update history
		self.history.append(text)
		self.history_pos = len(self.history)-1
		
		self.redirectstd()
		sys.stdout.write("\n")
		action = self.core.push(text)
		if action == 0:
			sys.stdout.write(PS1)
		elif action == 1:
			sys.stdout.write(PS2)
		self.restorestd()
		self.view.scroll_mark_onscreen(self.buffer.get_insert())


	def quit(self,*args):
		if __name__ == '__main__':
			gtk.main_quit()
		else:
			if self.popup != None:
				self.popup.hide()
			self.gui.hide()


if __name__ == '__main__':
	shell = Shell_Gui(with_window = 1)
	gtk.main()

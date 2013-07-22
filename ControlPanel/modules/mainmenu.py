from .base.module import cpModule
from .base.module import mroute,mview
from .base.bottle import view
from .base.bottle import static_file
from .base.bottle import HTTPError
from .base.bottle import redirect

import os

__all__ = [ 'cpMainMenu' ] 

class cpMainMenu(cpModule):
	def __init__(self, mountRoot="/"):
		super().__init__(mountRoot)
		self.mMenuItems = []
		self.setNavMenu([["Main", mountRoot ]])

	@mroute("/")
	@mview('menu')
	def view(self):
		return dict(layout_actions = [], menu_items=self.mMenuItems)
	
	def addItem(self, name, icon, path):
		self.mMenuItems += [[name, path, icon]]

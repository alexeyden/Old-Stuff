from .base.module import cpModule
from .base.auth import cpAuth
from .base.module import mroute,mview
from .base.bottle import view
from .base.bottle import static_file
from .base.bottle import HTTPError
from .base.bottle import redirect
from .base.bottle import request
from .base.bottle import response

import os

__all__ = [ 'cpAuthForm' ] 

class cpAuthForm(cpModule):
	def __init__(self, mountRoot="/auth"):
		super().__init__(mountRoot, False)
		self.setNavMenu([["Auth", mountRoot ]])

	@mroute("/")
	@mview('form')
	def view(self):
		return dict(
			form_title = "Auth",
			form_action = self.getMountRoot(),
			form_items = [
				['text', "Password: ", "password" ],
				['submit', 'Login', 'submit' ]
			]
		)
	
	@mroute("/", method="POST")
	@mview('form')
	def do_login(self):
		if cpAuth.auth(request.forms.get('password')):
			response.set_cookie('auth',request.forms.get('password'))
			return dict(
				form_title = "Auth",
				form_action = self.getMountRoot(),
				form_items = [
					['label', 'Auth is OK']
				]
			) 
		else:
			return view(self)	

	def addItem(self, name, icon, path):
		self.mMenuItems += [[name, path, icon]]

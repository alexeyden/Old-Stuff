from .bottle import Bottle
from .bottle import template
from .auth import cpAuth
from .bottle import redirect

__all__ = [ 'cpModule', 'mroute', 'mview' ]

class cpModule(object):
	mRoot = None

	def __new__(cls, *args, **kwargs):
		new_obj = super(cpModule,cls).__new__(cls)
		new_obj.mApp = Bottle()

		for kw in dir(new_obj):
			attr = getattr(new_obj, kw)
			if hasattr(attr, 'route_path'):
				for (idx, pth) in enumerate(attr.route_path):
					new_obj.mApp.route(pth, attr.route_method)(attr)
		return new_obj

	def __init__(self,mountRoot, useAuth = True):
		self.mNavigationMenu = []
		self.mMountRoot = mountRoot
		self.mUseAuth = useAuth
	
	def setAsRoot(self, app):
		cpModule.mRoot = self
		app.merge(self.mApp)
	
	def setNavMenu(self,new_menu):
		self.mNavigationMenu = new_menu
	
	def getMountRoot(self):
		return self.mMountRoot
	
	def attachViews(self,app):
		app.mount(self.mMountRoot,self.mApp)

def mroute(route, method="GET"):
	def decorator(func):
		if hasattr(func,'route_path'):
			func.route_path.append(route)
			func.route_method.append(method)
		else:
			func.route_path = [route]
			func.route_method = [method]
		return func
	return decorator 

def mview(templ):
	def decorator(func):
		def handle_params(self,*args,**kwords):
			if self.mUseAuth and not cpAuth.check():
				redirect(cpAuth.mForm.mMountRoot)
				return template("message", auth_message = "Redirecting")
			else:
				return template(
					templ,
					layout_title = 'Control Panel',
					layout_nav_menu = cpModule.mRoot.mNavigationMenu + (self.mNavigationMenu if cpModule.mRoot is not self else []),
					**func(self,*args,**kwords))
		return handle_params
	return decorator

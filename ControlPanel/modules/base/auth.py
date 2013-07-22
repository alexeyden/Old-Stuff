from .bottle import request
from .bottle import response
import hashlib

__all__ = [ 'cpAuth' ] 

class cpAuth:
	def check():
		md5hash = hashlib.md5()
		md5hash.update(bytes(request.cookies.get('auth','none'),'utf-8'))
		pass_hash = md5hash.hexdigest()
		if pass_hash == cpAuth.mHash:
			return True
		else:
			return False

	def auth(pswd):
		md5hash = hashlib.md5()
		md5hash.update(bytes(pswd,'utf-8'))
		pass_hash = md5hash.hexdigest() 
		if pass_hash == cpAuth.mHash:
			return True
		else:
			return False

	def reset():
		cpAuth.mAuthorized = False
		
	def setHash(pass_hash_str):
		cpAuth.mHash = pass_hash_str
	
	def setForm(form):
		cpAuth.mForm = form
	
	mHash = None

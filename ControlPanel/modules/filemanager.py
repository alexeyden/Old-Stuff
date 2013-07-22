from .base.module import cpModule
from .base.module import mroute,mview
from .base.bottle import view
from .base.bottle import static_file
from .base.bottle import HTTPError
from .base.bottle import redirect 
from .base.bottle import request

import threading
import tarfile
import os
import tarfile

__all__ = [ 'cpFileManager' ] 

mutex = threading.Lock()

class TarPacker(threading.Thread):
	def __init__(self, path, out_path, callerObj):
		threading.Thread.__init__(self)
		self.mPath = path
		self.mOutPath = out_path
		self.mCallerObj = callerObj 

	def run(self):
		if os.path.exists(self.mOutPath + TarPacker.mFileName):
			os.unlink(self.mOutPath + TarPacker.mFileName)

		tar = tarfile.open(self.mOutPath + TarPacker.mFileName, 'w')
		tar.add(self.mPath)
		tar.close()
		
		mutex.acquire()
		self.mCallerObj.onThreadDone()
		mutex.release()
	
	mFileName = 'cp-filemanager-dir.tar'

class cpFileManager(cpModule):
	mBrowse = 1
	mCopy = 2
	mPaste = 3
	mDelete = 4

	def __init__(self,filesRoot,mountRoot='/filemanager/'):
		super().__init__(mountRoot)
		self.mFilesRoot = filesRoot
		self.mTmpRoot = 'static/tmp/'
		self.setNavMenu([['File Manager',self.mMountRoot]])

		self.mCurrentPath = ''
		
		self.mMode = cpFileManager.mBrowse
		self.mToCopy = ''		

		self.mTarProcessing = False
		self.mTarStarted = False
		self.mTarPath = 'static/tmp/'
	
	@mroute('/download/<filepath:path>')
	def download(self,filepath):
		return static_file(filepath, root = self.mFilesRoot, download = True)
	
	@mroute('/tmp/<filepath:path>')
	def download_tmp(self, filepath):
		print(filepath)
		print(self.mTmpRoot)
		return static_file(filepath, root = self.mTmpRoot, download = True)

	@mroute('/do_upload', method='POST')
	@mview('form')
	def onUpload(self):
		upload = request.files.get('upload')
		try:
			upload.save(self.mFilesRoot + self.mCurrentPath)
		except IOError as err:
			return dict(form_title = 'Upload', form_action = self.mMountRoot, form_items = [
				['label', 'Error: ' + '{0}'.format(err)]
			])

		return dict(form_title = 'Upload', form_action = self.mMountRoot, form_items = [
			['label', 'Upload is done']
		])

	def onThreadDone(self):
		self.mTarProcessing = False	
	
	@mroute('/do_tar')
	@mview('waiter')
	def onMakeTar(self):
		mutex.acquire()
		is_running = self.mTarProcessing
		mutex.release()
		if not is_running:
			redirect(self.mMountRoot + 'action/tar')
		else:
			return dict(wait_msg = 'Creating *.tar file. Please wait...')

	@mroute('/copy/<pathname:path>')
	def onCopy(self, pathname):
		self.mToCopy = pathname	
		self.mMode = mBrowse
		redirect(self.mMountRoot + 'browse/' + self.mCurrentPath)
	
	@mroute('/action/<action:re:[a-z]+>')
	@mview('form')
	def onAction(self, action):
		if action == 'upload':
			return dict(form_title = 'Upload', enctype='multipart/form-data', form_action = self.mMountRoot + 'do_upload', form_items = [
				['file', "Select a file: ", "upload"],
				['submit', 'Submit', 'submit']	
			])
		elif action == 'tar':
			mutex.acquire()
			is_running = self.mTarProcessing
			mutex.release()

			if not self.mTarStarted or (self.mTarStarted and is_running):
				if not self.mTarStarted:
					self.mTarStarted = True
					self.mTarProcessing = True
					tar = TarPacker(self.mFilesRoot + self.mCurrentPath, self.mTarPath, self)
					tar.start()
				redirect(self.mMountRoot + 'do_tar')
			
			if not is_running:
				self.mTarStarted = False
				return dict(form_title = 'TAR', form_action = None, form_items = [
					['label', 'Packing is done. Download link: '],
					['link', self.mMountRoot + 'tmp/' + TarPacker.mFileName, "Download"]
				])	
		elif action == 'copy':
			self.mMode = cpFileManger.mCopy
			redirect(self.mMountRoot + 'browse/' + self.mCurrentPath)
		
		return dict(form_title = 'You should never get here',form_action = self.mMountRoot, form_items = [])

	@mroute('/browse/<pathname:path>')
	@mroute('/')
	@mview('menu')
	def view(self,pathname=''):	
		if not os.path.exists(self.mFilesRoot + pathname):
			raise HTTPError(status=404,body="Resource not found")
		
		if os.path.isfile(self.mFilesRoot + pathname):
			redirect(self.mMountRoot + 'download/' + pathname)
		else:
			self.mCurrentPath = pathname	

		file_list = os.listdir(self.mFilesRoot + pathname)
		dir_content = []
		
		if pathname != '' and pathname[0] != '/':
			pathname = '/' + pathname

		for filename in file_list:
			icon = 'document.png'
			if(os.path.isdir(self.mFilesRoot + pathname + filename)):
				icon = 'folder.png'

			if self.mMode == cpFileManager.mBrowse:
				link = self.mMountRoot + 'browse'  + pathname + '/' + filename
			
			elif self.mMode == cpFileManager.mCopy:
				link = self.mMountRoot + 'copy' + pathname + '/' + filename
			
			if(os.path.isdir(self.mFilesRoot + pathname + filename)):
				link = link + '/'
				
			if(len(filename) > 10):
				filename = filename[0:8] + ".."

			dir_content.append([filename,link, icon])

		if(not os.path.samefile(self.mFilesRoot + pathname,self.mFilesRoot)):
			dir_content.insert(0,['Up',self.mMountRoot,'arrow-up.png'])


		if self.mMode == cpFileManager.mBrowser:
			return dict(layout_actions = [
					[self.mMountRoot + 'action/upload','upload'],
					[self.mMountRoot + 'action/tar','tar'],
					[self.mMountRoot + 'action/copy', 'copy'],
					[self.mMountRoot + 'action/paste', 'paste'],
					[self.mMountRoot + 'action/rename', 'rename'],
					[self.mMountRoot + 'action/delete', 'delete'],
					[self.mMountRoot + 'action/new_folder', 'new folder']
				],
				menu_items = dir_content
			)
		else:
			return dict(layout_actions = [], menu_items = dir_content)


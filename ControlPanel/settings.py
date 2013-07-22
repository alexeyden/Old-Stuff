from modules.filemanager import cpFileManager;

settings = dict (
	style = "grey",
	pswd_hash = '4e64ea76fcad6497a969173914453f51' 
)
 
modules = [
	[ "Files", "folder.png", cpFileManager("./static/files/") ],
	[ "Bookmarks","agenda.png","/" ],
	[ "Music","headphones.png","/" ],
]

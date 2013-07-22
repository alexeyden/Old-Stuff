from modules.base.bottle import route,run,view
from modules.base.bottle import Bottle
from modules.base.bottle import static_file
from modules.base.bottle import template
from modules.base.bottle import redirect

from modules.base.module import cpModule
from modules.filemanager import cpFileManager
from modules.mainmenu import cpMainMenu
from modules.base.auth import cpAuth
from modules.auth import cpAuthForm

import settings

app = Bottle()

@app.route('/style/<filepath:path>')
def style(filepath):
	return static_file(filepath, root='static/styles/' + settings.settings["style"])

mainMenu = cpMainMenu()
mainMenu.setAsRoot(app)

authForm = cpAuthForm()
authForm.attachViews(app)
cpAuth.setForm(authForm)
cpAuth.setHash(settings.settings['pswd_hash'])

for item in settings.modules:
	if not isinstance(item[2], str):
		mainMenu.addItem(item[0], item[1], item[2].getMountRoot())
		item[2].attachViews(app)
	else:
		mainMenu.addItem(item[0], item[1], item[2])

run(app,host='0.0.0.0',port=8080,debug=True)

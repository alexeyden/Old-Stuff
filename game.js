var canvas;
requestAnimationFrame = window.requestAnimationFrame || window.mozRequestAnimationFrame ||  
                       window.webkitRequestAnimationFrame || window.msRequestAnimationFrame;

var game = {
    BLOCK_SIZE : 16, MAP_SIZE : 96, SCREEN_SIZE : 32,
    start_level : "level1.txt",
    maps_path : "data/maps/txt/",
    tiles_path : "data/tiles/objects/",
    tilesets_path : "data/tiles/tilesets/",
    items_path : "data/items.txt",
    skills_path : "data/skills.txt",

    tileset_tiles : ["water.png","tree.png","blocking.png","ground1.png","ground2.png"],
    object_tiles : ["teleport.png","sign.png","item.png","building.png"],
    object_images : [],
    monster_tiles : ["mob_snake.png","mob_skeleton.png","mob_orc.png","mob_dog.png","mob_boss1.png","mob_boss2.png"],
    monster_images : [],
    player_tile : "player.png",
    
    items : [],
    skills : [],
    
    total_images : 11,
    loaded_images : 0,

    update_screen : true,
    animation_lock : false,
    
    K_MOVE_LEFT : 65,
    K_MOVE_RIGHT : 68,
    K_MOVE_UP : 83,
    K_MOVE_DOWN : 87,
    K_DEBUG: 73,
    K_PICKUP: 80,

    LEVEL_DELTA_XP: 10,
}
game.player = {
    x: 0, y: 0,
    
    health: 10, power: 5,
    max_health: 10, max_power: 5,
    str: 1, con: 0, dex: 0,
    xp: 0, level: 1,

    inventory: [],
    clothes: [undefined,undefined,undefined,undefined,undefined],
    weapon : undefined, 

    CLOTH_HEAD : 0,
    CLOTH_BODY : 1,
    CLOTH_LEGS : 3,
    CLOTH_FEET : 4
}
game.map = {
    path : "",
    tileset : "",
    images : [],
    data : [],
    objects: [],

    OBJ_TYPE_TELEPORT : 0,
    OBJ_TYPE_SIGN : 1,
    OBJ_TYPE_ITEM : 2,
    OBJ_TYPE_SHOP : 3,
    OBJ_TYPE_MONSTER : 4
}
map_object = function(x,y,type,params)
{
    var map_obj  = new Object();
    map_obj.x = x; map_obj.y = y;
    map_obj.type = type; map_obj.params = params;
    return map_obj;
}
item = function(_type,_level,_name,_params)
{
    var new_item = {
	level : _level,
	type : _type,
	name : _name,
 	params : _params
    }
    new_item.toString = function()
    {
	return this.name + ' (' + 
	((this.params[0]>0) ? '+':'') + this.params[0] + ' ' +
	((this.params[1]>1) ? '+':'') + this.params[1] + ')';
    }
    return new_item;
}
skill = function(_name,_tip,_power,_func)
{
    var new_skill = {
	name : _name,
	tip : _tip,
	power : _power,
	func : _func
    }
    return new_skill;
}
game.parse_file = function(file_path)
{
    /*TODO: check success */
    var split_re = /\s(?![A-z_\-]+\")/;

    var xmlhttp = new XMLHttpRequest();
    xmlhttp.open('GET', file_path, false);
    xmlhttp.send(null);
    var content = xmlhttp.responseText;
    var lines = content.split('\n');

    var records = []
    for(var line=0;line < lines.length; line++)
    {
	var params = []
	if(lines[line][0] == '#')
	    params = lines[line].split(split_re);
	if(params.length > 0)
	    records.push(params);
    }
    return records;
}
game.ui = { active_item: NaN }
game.ui.update_prop = function(prop_name,value) {
    document.getElementById(prop_name).innerHTML=value;
}
game.ui.debug_panel_toggle = function()
{
    debug_panel = document.getElementById("debug_info");
    debug_panel.hidden=!debug_panel.hidden;
}
game.ui.help_panel_toggle = function()
{
    help_panel = document.getElementById("help_panel");
    help_panel.hidden=!help_panel.hidden;
}
game.ui.log_add = function(message)
{
    log = document.getElementById("log");
    log.value+=("> " + message+"\n");
    log.selectionStart=log.value.length-1;
}
game.ui.log_clear = function() { document.getElementById("log").value=''; }
game.ui.inventory_add = function(item_index,item_id)
{
    inv_menu = document.getElementById("menu_inventory");

    if(inv_menu.firstElementChild == null)
	inv_menu.innerHTML = '';

    item_str = "<a href='#' id='item_" + item_id +"' onclick='game.ui.inventory_toggle_info("+item_id+");'>" +
	game.items[item_index].toString() + "</a><br>";
    inv_menu.innerHTML+=item_str;
}
game.ui.inventory_clear = function(it)
{
    inv_menu = document.getElementById("menu_inventory");
    inv_menu.innerHTML = 'empty';
}

game.ui.inventory_toggle_info = function(item_id)
{
    inv_panel = document.getElementById("inv_panel");
    inv_info = document.getElementById("inv_info");

    if(inv_panel.hidden)
    {
	it = game.items[game.player.inventory[item_id]]
	type_label = it.type
	param_labels=[]
	switch(it.type) {
	case 'weapon': param_labels = ["Strength","Dexterity"]; break;
	case 'cloth': param_labels = ["Constitution","Dexterity"]; type_label+=" ("+it.params[2]+")"; break;
	case 'potion': param_labels = ["Health", "Power"]; break;
	}
	inv_info.innerHTML = "<b>" + it.name + "</b><br>" + type_label + "<br>" +
	    ((it.params[0]!=0) ? (((it.params[0] > 0) ? '+':' ') + it.params[0] + " " + param_labels[0] + "<br>") : '') +
	    ((it.params[1]!=0) ? (((it.params[1] > 0) ? '+':' ') + it.params[1] + " " + param_labels[1] + "<br>") : '') +
	    ((it.params[0]==0 && it.params[1]==0) ? 'nothing' : '');
	this.active_item = item_id;
    }
    else this.active_item = NaN;

    inv_panel.hidden=!inv_panel.hidden;
}
game.ui.inventory_drop = function(item_id)
{
    game.map.add_object(game.player.x,game.player.y,
			game.map.OBJ_TYPE_ITEM,
			[0, game.player.inventory[item_id]]);
    game.player.item_remove(item_id);
    game.ui.inventory_toggle_info(NaN);
    game.update_screen = true;
}
game.ui.inventory_use = function(item_id)
{
    
}

game.on_image_load = function() { game.loaded_images++; }
game.on_key_press = function(event)
{
    switch(event.which)
    {
    case game.K_MOVE_LEFT: game.player.step(-1,0); break;
    case game.K_MOVE_RIGHT: game.player.step(1,0); break;
    case game.K_MOVE_DOWN: game.player.step(0,-1); break;
    case game.K_MOVE_UP: game.player.step(0,1); break;
    case game.K_PICKUP:
	obj_idx = game.map.object_at(game.player.x,game.player.y);
	obj = game.map.objects[obj_idx];
	if(obj)
	{
	    if(obj.type == game.map.OBJ_TYPE_ITEM)
	    {
		if(obj.params[0] != 0) //random item
		{
		    ids = []
		    for(var i=0;i<game.items.length;i++)
			if(game.items[i].level == obj.params[0]) ids.push(i);

		    rnd_id = Math.floor((Math.random()*10000)%(ids.length+1));

		    if(rnd_id==0 || ids.length==0)
			game.ui.log_add("Box is empty.");
		    else
		    {
			game.player.item_add(ids[rnd_id-1]);
			game.ui.log_add("You got " + game.items[ids[rnd_id-1]].name); 
		    }
		}
		else
		{
		    game.player.item_add(obj.params[1]);
		    game.ui.log_add("You got " + game.items[obj.params[1]]);
		}
		game.map.objects.splice(obj_idx,1);
	    }
	    else
		game.ui.log_add("You cannot pick it up");
	    
	    game.update_screen = true;
	}
	else
	    game.ui.log_add("There is nothing here to pick up");
	break;
    case game.K_DEBUG:
	var s = "#some 1 2 no-quotes 3 \"text param\" \"string\" 100 19";
	var re2 = /\s(?![A-z_\-]+\")/;
	//alert('['+s.split(re2)+']');
	co = game.parse_file(game.items_path)
	for(var c = 0; c < co.length; c++)
	{
	    alert(co[c]);
	}
	break;
    default: return true;
    }
    return false;
}

game.load_items = function()
{
    items = this.parse_file(this.items_path);

    for(var i = 0; i < items.length; i++)
    {
	params = [parseInt(items[i][3],10),parseInt(items[i][4],10)]
	if(items[i][0] == "#cloth")
	    params.push(items[i][5]) //cloth type
	this.items.push(new item(items[i][0].slice(1),
				 parseInt(items[i][1]),
				 items[i][2].substr(1,items[i][2].length-2),params));
    }
}

game.player.reset = function()
{
    this.health = 100; this.mana = 100;
    this.x = 0; this.y = 0;
}

game.player.item_add = function(item_index) {
    this.inventory.push(item_index);
    game.ui.inventory_add(item_index,this.inventory.length-1);
}
game.player.item_remove = function(id) {
    this.inventory.splice(id,1);
    game.ui.inventory_clear();
    //TODO: something clever
    for(var i = 0; i < this.inventory.length; i++)
	game.ui.inventory_add(this.inventory[i],i);
}

game.player.step = function(dx,dy)
{
    if((this.x+dx < game.MAP_SIZE) && (this.x+dx >= 0) &&
       (this.y+dy < game.MAP_SIZE) && (this.y+dy >= 0))
	if(game.map.is_walkable(this.x+dx,this.y+dy))
	{
	    this.x+=dx;
	    this.y+=dy;
	    game.ui.update_prop("stat_player_pos",this.x + ", " + this.y);
	    game.update_screen = true;
	    
	    obj = game.map.objects[game.map.object_at(this.x,this.y)];
	    if(!obj)
		return;

	    switch(obj.type)
	    {
	    case game.map.OBJ_TYPE_TELEPORT:
		if(typeof(obj.params[0]) == "number")
		{
		    for(var j=0;j < game.map.objects.length; j++)
		    {
			if((game.map.objects[j].type) == game.map.OBJ_TYPE_TELEPORT &&
			   (game.map.objects[j].params[0] == obj.params[1]))
			{
			    this.x = game.map.objects[j].x;
			    this.y = game.map.objects[j].y;

			    game.ui.log_add("Teleported");
			}
		    }
		}
		else
		    game.map.load(map.objects[i].params[0]);
		break;
	    case game.map.OBJ_TYPE_SIGN:
		game.ui.log_add("You see a sign. There is text on this sign: \"" + obj.params[0] + "\"");
		break;
	    case game.map.OBJ_TYPE_ITEM:
		game.ui.log_add("You see the box.");
		break;
	    }
	}
}
game.map.is_walkable = function(x,y)
{
    if(this.data[x][y] > 2) return true;
    return false;
}
game.map.add_object = function(x,y,type,params)
{
    var new_obj = new map_object(x,y,type,params);
    this.objects.push(new_obj);
}
game.map.object_at = function(x,y)
{
    for(var obj=0; obj < this.objects.length; obj++)
	if((this.objects[obj].x == x) && (this.objects[obj].y == y))
	    return obj;
    return null;
}
game.map.load = function(map_name)
{	
    this.path = game.maps_path + map_name;
    this.data = [];
    this.objects = [];

    map_data = game.parse_file(this.path);
    
    for(var i=0; i < map_data.length; i++)
    {
	switch(map_data[i][0])
	{
	case '#tileset':
	    this.tileset = map_data[i][1];
	    break;
	case '#data':
	    for(var row = 0; row < game.MAP_SIZE; row++)
	    {
		var new_row = [];
		for(var col = 0; col < game.MAP_SIZE; col++)
		    new_row.push(parseInt(map_data[i][1][col*game.MAP_SIZE+row],10));
		this.data.push(new_row);
	    }
	    break;
	case '#spawn':
	    game.player.x = parseInt(map_data[i][1],10);
	    game.player.y = parseInt(map_data[i][2],10);
	    break;
	case '#teleport':
	    pos = [parseInt(map_data[i][1],10),parseInt(map_data[i][2],10)];
	    id = parseInt(map_data[i][3],10);
	    id_dest = parseInt(map_data[i][4],10);
	    if(isNaN(id_dest))
		id_dest = map_data[i][4];
	    this.add_object(pos[0],pos[1],game.map.OBJ_TYPE_TELEPORT,[id,id_dest]);
	    break
	case '#sign':
	    pos = [parseInt(map_data[i][1],10),parseInt(map_data[i][2],10)];
	    this.add_object(pos[0],pos[1],game.map.OBJ_TYPE_SIGN,[map_data[i][3]])
	    break;
	case '#item':
	    pos = [parseInt(map_data[i][1],10),parseInt(map_data[i][2],10)];
	    this.add_object(pos[0],pos[1],game.map.OBJ_TYPE_ITEM,
			    [parseInt(map_data[i][3],10),parseInt(map_data[i][4],10)]);
	    break;
	case '#shop':
	    pos = [parseInt(map_data[i][1],10),parseInt(map_data[i][2],10)];
	    this.add_object(pos[0],pos[1],game.map.OBJ_TYPE_SHOP,[]);
	    break;
	case '#monster':
	    pos = [parseInt(map_data[i][1],10),parseInt(map_data[i][2],10)];
	    monster_type = map_data[i][3];
	    monster_level = parseInt(map_data[i][4],10);
	    this.add_object(pos[0],pos[1],game.map.OBJ_TYPE_MONSTER,[monster_type,monster_level]);
	    break;
	}
    }

    this.images = []
    for(var i = 0; i < game.tileset_tiles.length; i++)
    {
	this.images[i] = new Image();
	//this.images[i].onload = game.on_image_load;
	this.images[i].src = game.tilesets_path+this.tileset+'/'+game.tileset_tiles[i];
    }
    
    game.ui.update_prop("stat_player_pos",game.player.x + ", " + game.player.y);
    game.ui.update_prop("stat_map",map_name);

}

game.map.draw = function()
{
    var page = [];
    page[0] = Math.floor(game.player.x / game.SCREEN_SIZE)*game.SCREEN_SIZE;
    page[1] = Math.floor(game.player.y / game.SCREEN_SIZE)*game.SCREEN_SIZE;

    for(var r=0; r < game.SCREEN_SIZE; r++)
	for(var c=0; c < game.SCREEN_SIZE; c++)
	    canvas.drawImage(this.images[this.data[r+page[0]][c+page[1]]],r*game.BLOCK_SIZE,c*game.BLOCK_SIZE);

    for(var obj = 0; obj < this.objects.length; obj++)
    {
	if(this.objects[obj].type != this.OBJ_TYPE_MONSTER)
	    canvas.drawImage(game.object_images[this.objects[obj].type],
			     (this.objects[obj].x-page[0])*game.BLOCK_SIZE,
			     (this.objects[obj].y-page[1])*game.BLOCK_SIZE);
    }

    canvas.drawImage(game.player_image,(game.player.x-page[0])*game.BLOCK_SIZE,(game.player.y-page[1])*game.BLOCK_SIZE);
}

game.init = function()
{
    canvas = document.getElementById("canvas").getContext("2d");
    
    this.player_image = new Image();
    this.player_image.onload = this.on_image_load;
    this.player_image.src = this.tiles_path + this.player_tile;
   
    for(var i = 0; i < this.object_tiles.length; i++)
    {
	this.object_images[i] = new Image();
	this.object_images[i].onload = this.on_image_load;
	this.object_images[i].src = this.tiles_path + this.object_tiles[i];
    }
    for(var i = 0; i < this.monster_tiles.length; i++)
    {
	this.monster_images[i] = new Image();
	this.monster_images[i].onload = this.on_image_load;
	this.monster_images[i].src = this.tiles_path + this.monster_tiles[i];
    }
    
    document.onkeyup = game.on_key_press;
    
    this.load_items();
    this.map.load(this.start_level);
    
    this.ui.update_prop("stat_health",this.player.health + "/" + this.player.max_health);
    this.ui.update_prop("stat_power",this.player.power + "/" + this.player.max_power);
    
    this.ui.update_prop("stat_xp",this.player.level + "/" + this.player.xp+"/"+(this.player.level*this.LEVEL_DELTA_XP));
    
    this.ui.update_prop("stat_str",this.player.str);
    this.ui.update_prop("stat_con",this.player.con);
    this.ui.update_prop("stat_dex",this.player.dex);
    this.ui.log_clear();
    this.ui.log_add("Welcome!");
}

game.render = function ()
{
    if(this.loaded_images == this.total_images)
    {
	if(this.update_screen)
	{
	    this.map.draw();
	    this.update_screen = false;
	}
    }
    else
    {
	canvas.fillStyle = 'rgb(0,0,0);';
	canvas.fillRect(0,0,1+this.loaded_images*10,10);
    }
}

mainloop = function()
{
    game.render();
    requestAnimationFrame(mainloop);
}

start = function()
{
    game.init();
    mainloop();
}
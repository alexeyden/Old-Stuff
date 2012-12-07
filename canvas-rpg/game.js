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
    anim_x: 0, anim_y: 0,

    health: 100, power: 100,
    max_health: 100, max_power: 100,
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
    OBJ_TYPE_SHOP : 3
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
    var split_re = /\s(?![^\"]+[^\s\"]\")/;

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
    log.scrollTop = log.scrollHeight;
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

game.ui.inventory_toggle_info = function(inv_item_id)
{
    inv_panel = document.getElementById("inv_panel");
    inv_info = document.getElementById("inv_info");

    if(inv_panel.hidden)
    {
		it = game.items[game.player.inventory[inv_item_id]];
		item_id = game.player.inventory[inv_item_id];
		type_label = it.type;
		param_labels=[];
		put_on_clothes = false;
		switch(it.type) {
			case 'weapon': param_labels = ["Strength","Dexterity"]; break;
			case 'cloth':
				cloth_type = ["head","body","legs","feet"]
				put_on_clothes = (game.player.clothes[game.player.CLOTH_HEAD] == item_id) || 
					(game.player.clothes[game.player.CLOTH_BODY] == item_id) ||
					(game.player.clothes[game.player.CLOTH_LEGS] == item_id) ||
					(game.player.clothes[game.player.CLOTH_FEET] == item_id);
				param_labels = ["Constitution","Dexterity"];
				type_label+=" ("+cloth_type[it.params[2]]+")";
			break;
			case 'potion': param_labels = ["Health", "Power"]; break;
		}
		inv_info.innerHTML = "<b>" + it.name + "</b>" + 
			((game.player.weapon == item_id) ? " (in hands)" : "") +
			((put_on_clothes) ? " (put on)" : "") + 
			"<br>" + type_label + "<br>" +
		    ((it.params[0]!=0) ? (((it.params[0] > 0) ? '+':' ') + it.params[0] + " " + param_labels[0] + "<br>") : '') +
		    ((it.params[1]!=0) ? (((it.params[1] > 0) ? '+':' ') + it.params[1] + " " + param_labels[1] + "<br>") : '') +
		    ((it.params[0]==0 && it.params[1]==0) ? 'nothing' : '');
		game.ui.active_item = inv_item_id;
    }
    else game.ui.active_item = NaN;

    inv_panel.hidden=!inv_panel.hidden;
}
game.ui.inventory_drop = function(inv_item_id)
{
    game.map.add_object(game.player.pos_x(),game.player.pos_y(),
			game.map.OBJ_TYPE_ITEM,
			[0, game.player.inventory[inv_item_id]]);
    game.player.item_remove(inv_item_id);
    game.ui.inventory_toggle_info(NaN);
    game.update_screen = true;
}
game.ui.inventory_use = function(inv_item_id)
{
	item_id = game.player.inventory[inv_item_id];
	switch(game.items[item_id].type)
	{
		case 'weapon':
		{
			if(game.player.weapon != undefined)
				game.player.apply_item_powers(game.player.weapon,true);

			if(game.player.weapon == item_id)
			{
				game.player.weapon = undefined;
				game.ui.inventory_toggle_info(NaN);
				break;
			}

			game.player.weapon = item_id;
			game.player.apply_item_powers(item_id,false);
			game.ui.inventory_toggle_info(NaN);
		}
		break;
		case 'cloth':
		{
			cloth_type = game.items[item_id].params[2];


			if(game.player.clothes[cloth_type]!=undefined)
				game.player.apply_item_powers(game.player.clothes[cloth_type],true);
			if(game.player.clothes[cloth_type] == item_id)
			{
				game.player.clothes[cloth_type] = undefined;
				game.ui.inventory_toggle_info(NaN);
				break;
			}
			game.player.clothes[cloth_type] = item_id
			game.player.apply_item_powers(item_id,false)
			game.ui.inventory_toggle_info(NaN)
		}
		break;
		case 'scroll':
		{

		}
		break;
		case 'potion':
		{
			game.player.apply_item_powers(item_id,false);
			game.player.item_remove(inv_item_id);
		}
		break;
	}
}

animator = {
    objects : [],
    animations : [],
    add : function(obj,dest_x,dest_y,speed)
    {
		animation = {
			anim_x : dest_x,
			anim_y : dest_y,
			anim_speed : speed
		}
		var found = false;
		var i = 0;
		for(; i < this.objects.length; i++)
		{
			if(this.objects[i] == obj)
			{
				found = true;
				break;
			}
		}

		if(found)
		{
			this.animations[i].push(animation);
		}
		else
		{
			this.objects.push(obj);
			this.animations.push([animation]);
		}

		game.animation_lock = true;
    },
    step : function(time)
    {
		for(var i = 0; i < this.objects.length; i++)
		{
			obj = this.objects[i];
			
			obj_x = obj.pos_x() - game.map.page[0]*game.SCREEN_SIZE;
			obj_y = obj.pos_y() - game.map.page[1]*game.SCREEN_SIZE;
			game.map.draw_tile(obj_y,   obj_x);
			game.map.draw_tile(obj_y+1, obj_x+1);
			game.map.draw_tile(obj_y+1, obj_x);
			game.map.draw_tile(obj_y,   obj_x+1);
		}
		
		for(var i = 0; i < this.objects.length; i++)
		{
			obj = this.objects[i];
			anim = this.animations[i][0];
			
			if(Math.abs(obj.x-anim.anim_x)<=anim.anim_speed && Math.abs(obj.y - anim.anim_y)<=anim.anim_speed)
			{
				obj.x = anim.anim_x;
				obj.y = anim.anim_y;
				game.map.draw();
				obj.draw();

				if(this.animations[i].length == 1)
				{
					this.objects.splice(i,1);
					this.animations.splice(i,1);
				}
				else
					this.animations[i].splice(0,1);
				
				if(this.objects.length == 0)
					game.animation_lock = false;
				
				if(obj.end_move) obj.end_move();

				break;
			}
				
			if(obj.x < anim.anim_x) obj.x+=anim.anim_speed;
			else if(obj.x > anim.anim_x) obj.x-=anim.anim_speed;
			if(obj.y < anim.anim_y) obj.y+=anim.anim_speed;
			else if(obj.y > anim.anim_y) obj.y-=anim.anim_speed;

			obj.draw();
		}
		game.player.draw();
    }
}
game.on_image_load = function() { game.loaded_images++; }
game.on_key_press = function(event)
{
    if(game.animation_lock)
	return false;

    switch(event.which)
    {
    case game.K_MOVE_LEFT: game.player.begin_move(-1,0); break;
    case game.K_MOVE_RIGHT: game.player.begin_move(1,0); break;
    case game.K_MOVE_DOWN: game.player.begin_move(0,-1); break;
    case game.K_MOVE_UP: game.player.begin_move(0,1); break;
    case game.K_PICKUP:
		obj_idx = game.map.object_at(game.player.pos_x(),game.player.pos_y());
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
    	/*
		var s = "#some 1 2 no-quotes 3 \"has quotes\" \"some more spaces\" 100 19 \"it works!\"";
		var re2 = /\s(?![^\"]+[^\s\"]\")/;
		alert('['+s.split(re2)+']');
		*/
		//game.player.item_add(4);
		//game.player.item_add(5);
		animator.add(game.player,game.player.x+16,game.player.y,4);
		animator.add(game.player,game.player.x,game.player.y,2);
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
		{
		    cloth_param = undefined
		    switch(items[i][5])
		    {
		    	case 'head': cloth_param = game.player.CLOTH_HEAD; break;
		    	case 'body': cloth_param = game.player.CLOTH_BODY; break;
		    	case 'legs': cloth_param = game.player.CLOTH_LEGS; break;
		    	case 'feet': cloth_param = game.player.CLOTH_FEET; break;
		    }
		    params.push(cloth_param);
		}
		this.items.push(new item(items[i][0].slice(1),
					 parseInt(items[i][1]),
					 items[i][2].substr(1,items[i][2].length-2),params));
    }
}

game.monster_manager = {
	monsters: [],
	SNAKE: 0,
	SKELETON: 1,
	ORC: 2,
	DOG: 3,
	BOSS_KNIGHT: 4,
	BOSS_MAGE: 5,
	add : function(id,_x,_y,_level)
	{
		new_monster = {
			type: id,
			x: _x*game.BLOCK_SIZE,
			y: _y*game.BLOCK_SIZE,
			level: _level,
			draw: function()
			{
				canvas.drawImage(game.monster_images[this.type],
					this.x-game.map.page[0]*game.SCREEN_SIZE*game.BLOCK_SIZE,
					this.y-game.map.page[1]*game.SCREEN_SIZE*game.BLOCK_SIZE);
			},
			pos_x: function() { return Math.floor(this.x/game.BLOCK_SIZE); },
			pos_y: function() { return Math.floor(this.y/game.BLOCK_SIZE); }
		}

		this.monsters.push(new_monster);
	},
	remove : function(x,y)
	{

	},
	step : function()
	{
		if(game.animation_lock)
			return;
		for(var i=0; i < this.monsters.length; i++)
		{
			if(game.map.is_visible(this.monsters[i].pos_x(),this.monsters[i].pos_y()))
			{
				dirs = [[1,0],[-1,0],[0,1],[0,-1]];
				attack = false;
				di = 0;
				for(; di < 4; di++)
				{
					if(game.player.pos_x() == (this.monsters[i].pos_x() + dirs[di][0]) &&
						game.player.pos_y() == (this.monsters[i].pos_y() + dirs[di][1]))
					{
						attack = true;
						break;
					}
				}
				if(attack)
				{
					animator.add(this.monsters[i],
						this.monsters[i].x+dirs[di][0]*game.BLOCK_SIZE/2,this.monsters[i].y+dirs[di][1]*game.BLOCK_SIZE/2,2);
					animator.add(this.monsters[i],this.monsters[i].x,this.monsters[i].y,2);
					continue;
				}	
				var dx = 0;
				var dy = 0;
				rnd_dim = Math.floor((Math.random()*100)%6);
				switch(rnd_dim){
					case 0: dx =  1;  break;
					case 1: dx = -1;  break;
					case 2: dy =  1;  break;
					case 3: dy = -1;  break;
				}
				if(game.map.is_walkable(this.monsters[i].pos_x() + dx,this.monsters[i].pos_y() + dy) && 
					!this.check_pos(this.monsters[i].pos_x() + dx,this.monsters[i].pos_y() + dy))
				{
					animator.add(this.monsters[i],this.monsters[i].x + dx*game.BLOCK_SIZE,this.monsters[i].y + dy*game.BLOCK_SIZE,2);
					//this.monsters[i].x+=dx*game.BLOCK_SIZE;
					//this.monsters[i].y+=dy*game.BLOCK_SIZE;
				}
			}
		}
	},
	draw : function()
	{
		for(var i = 0; i < this.monsters.length; i++)
			this.monsters[i].draw();
	},
	check_pos: function(x,y)
	{
		for(var i=0;i<this.monsters.length;i++)
			if(this.monsters[i].pos_x() == x && this.monsters[i].pos_y() == y)
				return i;
		return undefined;
	}
}
game.player.pos_x = function()
{
    return Math.floor(this.x / game.BLOCK_SIZE)
}
game.player.pos_y = function()
{
    return Math.floor(this.y / game.BLOCK_SIZE);
}
game.player.reset = function()
{
    this.health = 100; this.power = 100;
    this.x = 0; this.y = 0;
}
game.player.apply_item_powers = function(item_id,revoke)
{
	action = (revoke) ? -1 : 1;

	switch(game.items[item_id].type)
	{
		case 'weapon':
			this.str+=action*game.items[item_id].params[0];
			this.dex+=action*game.items[item_id].params[1];
			game.ui.update_prop("stat_str",this.str);
			game.ui.update_prop("stat_dex",this.dex);
			break;
		case 'cloth':
			this.con+=action*game.items[item_id].params[0];
			this.dex+=action*game.items[item_id].params[1];
			game.ui.update_prop("stat_con",this.con);
			game.ui.update_prop("stat_dex",this.dex);
			break;
		case 'potion':
			if(this.health + game.items[item_id].params[0] >= this.max_health)
				this.health = this.max_health;
			else
				this.health+=game.items[item_id].params[0];

			if(this.power + game.items[item_id].params[1] >= this.max_power)
				this.power = this.max_power
			else
				this.power+=game.items[item_id].params[1];
			
			game.ui.update_prop("stat_health",this.health + "/" + this.max_health);
			game.ui.update_prop("stat_power",this.power + "/" + this.max_power);
			break;
	}
}
game.player.item_add = function(item_index) {
    this.inventory.push(item_index);
    game.ui.inventory_add(item_index,this.inventory.length-1);
}
game.player.item_remove = function(id) {
	item_id = this.inventory[id];
	if(this.weapon == item_id)
	{
		this.weapon = undefined;
		this.apply_item_powers(item_id,true);
	}
	put_on_cloth = undefined
	if(this.clothes[this.CLOTH_HEAD] == item_id) put_on_cloth = this.CLOTH_HEAD;
	if(this.clothes[this.CLOTH_BODY] == item_id) put_on_cloth = this.CLOTH_BODY;
	if(this.clothes[this.CLOTH_LEGS] == item_id) put_on_cloth = this.CLOTH_LEGS;
	if(this.clothes[this.CLOTH_FEET] == item_id) put_on_cloth = this.CLOTH_FEET;
	if(put_on_cloth != undefined)
	{
		this.clothes[put_on_cloth] = undefined;
		this.apply_item_powers(item_id,true);
	}

    this.inventory.splice(id,1);
    game.ui.inventory_clear();

    for(var i = 0; i < this.inventory.length; i++)
		game.ui.inventory_add(this.inventory[i],i);
}

game.player.begin_move = function(dx,dy)
{
    if((this.pos_x()+dx < game.MAP_SIZE) && (this.pos_x()+dx >= 0) &&
       (this.pos_y()+dy < game.MAP_SIZE) && (this.pos_y()+dy >= 0))
    {
		if(game.map.is_walkable(this.pos_x()+dx,this.pos_y()+dy))
		{
			if(game.monster_manager.check_pos(this.pos_x()+dx,this.pos_y()+dy) != undefined)
			{
				return;
			}
		    animator.add(this,this.x+dx*game.BLOCK_SIZE,this.y+dy*game.BLOCK_SIZE,4);
		}
    }
}

game.player.end_move = function()
{
    game.ui.update_prop("stat_player_pos",this.pos_x() + ", " + this.pos_y());
	game.monster_manager.step();
	game.update_screen = true;

    obj = game.map.objects[game.map.object_at(this.pos_x(),this.pos_y())];
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
					this.x = game.map.objects[j].x*game.BLOCK_SIZE;
					this.y = game.map.objects[j].y*game.BLOCK_SIZE;

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
    
	game.update_screen = true;
}

game.player.draw = function()
{
    canvas.drawImage(game.player_image,
		     this.x-game.map.page[0]*game.SCREEN_SIZE*game.BLOCK_SIZE,
		     this.y-game.map.page[1]*game.SCREEN_SIZE*game.BLOCK_SIZE);
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
		    game.player.x = parseInt(map_data[i][1],10) * game.BLOCK_SIZE;
		    game.player.y = parseInt(map_data[i][2],10) * game.BLOCK_SIZE;
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
		    this.add_object(pos[0],pos[1],game.map.OBJ_TYPE_SIGN,
				    [map_data[i][3].substring(1,map_data[i][3].length-1)]);
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
		    monster_type_str = map_data[i][3];
		    monster_type_id = 0;

		    monster_types=["snake","skeleton","orc","dog","boss_knight","boss_mage"];
	
		    for(var j =0; j<monster_types.length; j++)
		    	if(monster_types[j]==monster_type_str)
		    		monster_type_id = j;

		    monster_level = parseInt(map_data[i][4],10);

		    game.monster_manager.add(monster_type_id,pos[0],pos[1],monster_level);
		    //this.add_object(pos[0],pos[1],game.map.OBJ_TYPE_MONSTER,[monster_type,monster_level]);
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

game.map.draw_tile = function(x,y)
{
	var tile = this.data[y+this.page[0]*game.SCREEN_SIZE][x+this.page[1]*game.SCREEN_SIZE];
    canvas.drawImage(this.images[tile],y*game.BLOCK_SIZE,x*game.BLOCK_SIZE);
}
game.map.is_visible = function(x,y)
{
	if(x < (this.page[0]+1)*game.SCREEN_SIZE &&
		x >= this.page[0]*game.SCREEN_SIZE &&
		y < (this.page[1]+1)*game.SCREEN_SIZE &&
		y >= this.page[1]*game.SCREEN_SIZE)
		return true;
	else
		return false;
}
game.map.draw = function()
{
    this.page = [];
    this.page[0] = Math.floor(game.player.pos_x() / game.SCREEN_SIZE);
    this.page[1] = Math.floor(game.player.pos_y() / game.SCREEN_SIZE);

    //draw tiles
    for(var r=0; r < game.SCREEN_SIZE; r++)
	for(var c=0; c < game.SCREEN_SIZE; c++)
	    this.draw_tile(c,r);

    //draw objects
    for(var obj = 0; obj < this.objects.length; obj++)
    {
		if(this.objects[obj].type != this.OBJ_TYPE_MONSTER)
			canvas.drawImage(game.object_images[this.objects[obj].type],
					(this.objects[obj].x-this.page[0]*game.SCREEN_SIZE)*game.BLOCK_SIZE,
					(this.objects[obj].y-this.page[1]*game.SCREEN_SIZE)*game.BLOCK_SIZE);
    }
    game.monster_manager.draw();
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

game.render = function(time)
{
    if(this.loaded_images == this.total_images)
    {
		if(this.update_screen)
		{
			this.map.draw();
			this.player.draw();
			this.update_screen = false;
		}

		animator.step(time);
    }
    else
    {
	canvas.fillStyle = 'rgb(0,0,0);';
	canvas.fillRect(0,0,1+this.loaded_images*10,10);
    }
}

mainloop = function(time)
{
    game.render(time);
    requestAnimationFrame(mainloop);
}

start = function()
{
    game.init();
    mainloop();
}
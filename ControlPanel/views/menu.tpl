<div id="menu">
	%if 'menu_message' in locals():
		<div class="frame">
			<div class="title">
				{{menu_message[0]}}
			</div>
			{{menu_message[1]}}
		</div>
	%else:
		%for item in menu_items:
		<div class="menu_item" onclick="location.href='{{item[1]}}';">
			<div class="menu_img">
				<img src="/style/images/menu/{{item[2]}}" class="menu_item">
			</div>
			<span class="menu_item">{{item[0]}}</span>
		</div>
		%end
	%end
</div>
%rebase layout title=layout_title,nav_menu=layout_nav_menu,actions=layout_actions

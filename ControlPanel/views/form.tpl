<div class="frame">
		<h1>{{form_title}}</h1>

		%if not 'enctype' in globals():
		<form action="{{form_action}}" method="POST">
		%else:
		<form action="{{form_action}}" method="POST" enctype="{{enctype}}">
		%end

			%for item in form_items:
				%if item[0] == 'label':
					{{item[1]}}<br>
				%elif item[0] == 'link':
					<a href="{{item[1]}}"> {{item[2]}} </a><br>
				%elif item[0] == 'submit':
					<input type="{{item[0]}}" name="{{item[2]}}" value="{{item[1]}}" />
				%else:
					{{item[1]}} <input type="{{item[0]}}" name="{{item[2]}}" /><br>
				%end
			%end
		</form>
</div>
%rebase layout title=layout_title,nav_menu=layout_nav_menu

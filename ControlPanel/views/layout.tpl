<!doctype html>
<html>

<head>
	<meta charset="utf-8">
	<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1">
	<title>{{title}}</title>
	<link rel="stylesheet" type="text/css" href="/style/style.css">
</head>

<body>
	<div class="frame">
		<div class="title">
			%if 'actions' in globals() and len(actions) != 0:
			<ul class="actions_menu">
				Actions
				%for act in actions:
					<li><a href="{{act[0]}}">{{act[1]}}</a></li>
				%end
			</ul>
			%end
			
			<ul class="path">
			%for nav_item in nav_menu:
				<li class="path">
					<a href="{{nav_item[1]}}" class="path">{{nav_item[0]}}</a>
				</li>
			%end
			</ul>
		</div>
			<div class="actions">
			
			</div>
		%include
	</div>
</body>

</html>


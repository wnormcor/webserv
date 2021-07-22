main = '''
<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="UTF-8">
	<meta http-equiv="X-UA-Compatible" content="IE=edge">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<link rel="shortcut icon" href="/images/favicon.ico" type="image/x-icon">
	<link rel="stylesheet" href="/style.css">
	<title>21-school</title>
</head>
<body>
	<header>
		<div class="logo">
			<a href="/index.html"><img src="/images/21_sber_logo_green.png" class="logo_image"></a>
			<p>project</p>
			<h1>webserv</h1>
		</div>
		<ol>
			<li><a href="https://profile.intra.42.fr/users/wnormcor" class="name"><img src="/images/avatarka.png" class="name_avatarka"><p class="name_text">wnormcor</p></a></li>
			<li><a href="https://profile.intra.42.fr/users/dbliss" class="name"><img src="/images/avatarka.png" class="name_avatarka"><p class="name_text">dbliss</p></a></li>
			<li><a href="https://profile.intra.42.fr/users/monie" class="name"><img src="/images/avatarka.png" class="name_avatarka"><p class="name_text">monie</p></a></li>
		</ol>
		<ol>
			<li><a href="https://github.com/wnormcor/webserv"><img src="/images/github.png" class="link"></a></li>
			<li><a href="https://www.instagram.com/codinggirl_/"><img src="/images/instagram.png" class="link"></a></li>
			<li><a href="https://www.youtube.com/channel/UClcioJjtFDB0shnRhxsUjog"><img src="/images/YouTube.png" class="link"></a></li>
		</ol>
	</header> 
	<main>
		{login}
		<ol class="talk">
			{posts}
		</ol>
		<div class="vacum"></div>
	</main>
	{publish}
</body>
</html>
'''

login = '''
<form class="authorization" action="/cgi-bin/wall.py">
	<h2 class="authorization_h2">Authorization or enter</h2>
	<input autofocus class="input authorization_input" size="100" maxlength="105" type="text" name="login"> 
	<input autofocus class="input authorization_input" size="100" maxlength="105" type="password" name="password"> 
	<button name="action" value="login">Sumbit</button>
</form>
'''

publish = '''
<footer class="footer">
	<form action="/cgi-bin/wall.py">
		<input autofocus class="input input_chat" name="text">
		<button class="button_chat" name="action" value="publish">&#8594;</button>
		<button class="button_delete" name="action" value="delete" ><p>Delete</p><p>chat</p></button>
	</form>
</footer>
'''
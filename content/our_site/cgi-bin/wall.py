#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import print_function
import sys
import cgi
import html
import http.cookies
import os
import cgi_content as cc

from cgi_def import Wall
wall = Wall()



def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

cookie = http.cookies.SimpleCookie(os.environ.get("HTTP_cookie"))
session = cookie.get("session")
if session is not None:
	session = session.value
user = wall.find_cookie(session)
# Ищем пользователя по переданной куке


# eprint("Session is: ", session)

form = cgi.FieldStorage()
action = form.getfirst("action", "")

# eprint("PRINT OF RESEVED FIELDS:")
# for i in form:
    # eprint(i, " and value is ", form.getfirst(i, ""))

def manager_content():
	if user is not None:
		log = ""
		pos = wall.html_list()
		pub = cc.publish
	else:
		log = cc.login
		pos = ""
		pub = ""

	if action == "delete":
		wall.d_c()
		log = cc.login
		pub = ""
		print('Content-Type: text/html\r\n\r\n')
		print(cc.main.format(login=log, posts="", publish=""))
		return
	elif action == "publish":
		text = form.getfirst("text", "")
		text = html.escape(text)
		if text and user is not None:
			wall.publish(user, text)
	elif action == "login":
		login = form.getfirst("login", "")
		login = html.escape(login)
		password = form.getfirst("password", "")
		password = html.escape(password)
		if wall.find(login, password):
			cookie = wall.set_cookie(login)
			print('Set-cookie: session={}'.format(cookie))
		elif wall.find(login):
			pass  # А надо бы предупреждение выдать
		else:
			wall.register(login, password)
			cookie = wall.set_cookie(login)
			print('Set-cookie: session={}'.format(cookie))

	print('Content-Type: text/html\r\n\r\n')
	print(cc.main.format(login=log, posts=pos, publish=pub))
	# print(cc.main.format(login=log, posts=wall.html_list(), publish=pub))

manager_content()
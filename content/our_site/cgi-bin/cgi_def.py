#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import json
import random
import time
import os


class Wall:
	USERS = 'users.json'
	WALL = 'wall.json'
	COOKIES = 'cookies.json'

	def __init__(self):
		"""Создаём начальные файлы, если они не созданы"""
		try:
			with open(self.USERS, 'r', encoding='utf-8'):
				pass
		except FileNotFoundError:
			with open(self.USERS, 'w', encoding='utf-8') as f:
				json.dump({}, f)

		try:
			with open(self.WALL, 'r', encoding='utf-8'):
				pass
		except FileNotFoundError:
			with open(self.WALL, 'w', encoding='utf-8') as f:
				json.dump({"posts": []}, f)

		try:
			with open(self.COOKIES, 'r', encoding='utf-8'):
				pass
		except FileNotFoundError:
			with open(self.COOKIES, 'w', encoding='utf-8') as f:
				json.dump({}, f)

	def register(self, user, password):
		"""Регистриует пользователя. Возвращает True при успешной регистрации"""
		if self.find(user):
			return False  # Такой пользователь существует
		with open(self.USERS, 'r', encoding='utf-8') as f:
			users = json.load(f)
		users[user] = password
		with open(self.USERS, 'w', encoding='utf-8') as f:
			json.dump(users, f)
		return True

	def set_cookie(self, user):
		"""Записывает куку в файл. Возвращает созданную куку."""
		with open(self.COOKIES, 'r', encoding='utf-8') as f:
			cookies = json.load(f)
		cookie = str(time.time()) + str(random.randrange(10**14))  # Генерируем уникальную куку для пользователя
		cookies[cookie] = user
		with open(self.COOKIES, 'w', encoding='utf-8') as f:
			json.dump(cookies, f)
		return cookie

	def find_cookie(self, cookie):
		"""По куке находит имя пользователя"""
		with open(self.COOKIES, 'r', encoding='utf-8') as f:
			cookies = json.load(f)
		return cookies.get(cookie)

	def find(self, user, password=None):
		"""Ищет пользователя по имени или по имени и паролю"""
		with open(self.USERS, 'r', encoding='utf-8') as f:
			users = json.load(f)
		if user in users and (password is None or password == users[user]):
			return True
		return False

	def publish(self, user, text):
		"""Публикует текст"""
		with open(self.WALL, 'r', encoding='utf-8') as f:
			wall = json.load(f)
		wall['posts'].append({'user': user, 'text': text})
		with open(self.WALL, 'w', encoding='utf-8') as f:
			json.dump(wall, f)

	def html_list(self):
		"""Список постов для отображения на странице"""
		with open(self.WALL, 'r', encoding='utf-8') as f:
			wall = json.load(f)
		posts = []
		for post in wall['posts']:
			i = 0
			content = '<li class="talk_window"><img src="/images/back.PNG" class="name_avatarka"><p class="talk_chat talk_dbiss">'
			content += 'Привет! Давай общаться!'
			content += '</p></li>'
			posts.append(content)
			content = '<li class="talk_window"><p class="talk_chat talk_user">'
			content += post['text']
			content += '</p><img src="/images/avatarka.png" class="name_avatarka">'
			posts.append(content)
		return '</li>'.join(posts)

	def d_c(self):
		"""delete_correspondence Удаляет всю историю"""
		path = os.path.join(os.path.abspath(os.path.dirname(__file__)), 'cookies.json')
		os.remove(path)
		path = os.path.join(os.path.abspath(os.path.dirname(__file__)), 'users.json')
		os.remove(path)
		path = os.path.join(os.path.abspath(os.path.dirname(__file__)), 'wall.json')
		os.remove(path)
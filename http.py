import socket
import utils

class client_t:
	def __init__(self,sock,addr,server):
		self.closed=False
		self.sock=sock
		self.addr=addr[0]+':'+str(addr[1])
		self.server=server
		self.reset()

	def reset(self):
		self.got_headers=False
		self.raw_headers=''
		self.headers=[]
		self.method=''
		self.request=''
		self.query_str=''
		self.queries=[]
		self.version=''
		self.content_size=0
		self.post_data=''
		self.got_post=False
		self.keepalive=False

	def update(self):
		if self.closed:
			return False

		data=''

		try:
			if utils.available(self.sock):
				data=self.sock.recv(1024)

				if not data:
					self.close()
					return False

		except Exception:
			self.close()
			return False

		if not self.got_headers:
			self.get_headers(data)
		elif not self.got_post:
			self.get_post(data)
		else:
			self.handle_request()

		return (not self.closed)

	def get_headers(self,data):
		self.raw_headers+=data
		find_index=self.raw_headers.find('\r\n\r\n')
		find_size=4

		if find_index<0:
			find_index=self.raw_headers.find('\n\n')
			find_size=2

		if find_index>=0:
			data=self.raw_headers[find_index+find_size:]
			self.raw_headers=self.raw_headers[:find_index]
			self.got_headers=True
			self.handle_headers(data)

	def handle_headers(self,data):
		first=True
		for line in self.raw_headers.split('\n'):
			line=line.strip()

			if first:
				first=False
				line=line.split()

				if len(line)!=2 and len(line)!=3:
					self.close()
					return

				self.method=line[0]
				self.request=line[1]

				if len(line)==3:
					self.version=line[2]

				self.query_str=self.request.split('?')

				if len(self.query_str)>1:
					self.request=self.query_str[0]
					self.query_str='?'.join(self.query_str[1:])
				else:
					self.query_str=''

				for ii in self.query_str.split('&'):
					ii=ii.split('=')
					if len(ii)>2:
						ii=[ii[0],'='.join(ii[1:])]
					if len(ii)==1:
						ii.append('true')
					if ii[0]!='':
						self.queries.append(ii)

			else:
				line=line.split(':')

				if len(line)==2:
					key=line[0].strip()
					value=line[1].strip()
					self.headers.append((key,value))

					if key.lower()=='connection':
						self.keepalive=(value.lower()=='keep-alive')
					elif self.method=='POST' and key.lower()=='content-length':
						try:
							self.content_size=min(int(value),2*1024*1024)

						except Exception:
							self.close()
							return
		self.get_post(data)

	def get_post(self,data):
		self.post_data+=data[0:self.content_size]

		if len(self.post_data)>=self.content_size:
			self.got_post=True
			self.handle_request()

	def handle_request(self):
		if self.server.handler:
			self.server.handler(self)

		if not self.closed:
			if self.keepalive:
				self.reset()
			else:
				self.close()

	def respond(self,code,msg,data='',ctype='text/html'):
		res=''

		if self.version=='HTTP/1.0' or self.version=='HTTP/1.1':
			res+=self.version+' '+str(code)+' '+msg+'\r\n'
			res+='Content-type: '+ctype+'\r\n'
			res+='Content-length: '+str(len(data))+'\r\n\r\n'
		res+=data

		try:
			self.sock.send(res)

		except Exception:
			pass

		if code!=200:
			self.close()

	def close(self):
		self.closed=True
		self.sock.close()

class server_t:
	def __init__(self,addr,port,handler=None):
		self.handler=handler
		self.clients=[]
		self.sock=socket.socket()
		self.sock.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
		self.sock.setsockopt(socket.SOL_SOCKET,socket.SO_KEEPALIVE,1)
		self.sock.bind((addr,port))
		self.sock.listen(1)

	def update(self):
		if utils.available(self.sock):
			new_sock,addr=self.sock.accept()
			self.clients.append(client_t(new_sock,addr,self))

		alive_clients=[]

		for client in self.clients:
			if client.update():
				alive_clients.append(client)

		self.clients=alive_clients
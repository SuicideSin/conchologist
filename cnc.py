import socket
import ssl
import utils
import uuid

class client_t:
	def __init__(self,server,sock,addr,uid):
		self.server=server
		self.sock=sock
		self.addr=addr[0]+':'+str(addr[1])
		self.uid=uid
		self.chunks=[]

	def send(self,data):
		self.chunks.append('$ '+data)
		try:
			self.sock.send(data)

		except Exception:
			self.close()

		if self.server.onsend:
			self.server.onsend(self)

	def recv(self):
		try:
			data=self.sock.recv(1024)

			if not data:
				self.close()
				return False

			self.add_recv_chunk(data)
			return True

		except Exception:
			return False

	def add_recv_chunk(self,data):
		if len(data)>0:
			self.chunks.append('  '+data)

			if self.server.onrecv:
				self.server.onrecv(self)

	def available(self):
		return utils.available(self.sock)

	def close(self):
		self.sock.close()

class server_t:
	def __init__(self,addr,port,certfile,keyfile):
		self.onopen=None
		self.onclose=None
		self.onrecv=None
		self.onsend=None
		self.clients={}
		self.ctx=ssl.SSLContext(ssl.PROTOCOL_SSLv23)
		self.ctx.load_cert_chain(certfile=certfile,keyfile=keyfile)
		self.sock=socket.socket()
		self.sock.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
		self.sock.setsockopt(socket.SOL_SOCKET,socket.SO_KEEPALIVE,1)
		self.sock.bind((addr,port))
		self.sock.listen(1)

	def available(self):
		return utils.available(self.sock)

	def update(self):
		if self.available():
			uid=str(uuid.uuid1())
			new_sock,addr=self.sock.accept()
			new_sock.settimeout(1)
			sig_data=''
			bad_ssl=False

			try:
				sig_data=new_sock.recv(0,socket.MSG_PEEK)
				ssl_client=client_t(self,self.ctx.wrap_socket(new_sock,server_side=True),addr,uid)
				self.clients[uid]=ssl_client

			except (socket.timeout,ssl.SSLError) as error:
				if str(error).find('[SSL: UNKNOWN_PROTOCOL] unknown protocol')==0 or str(error).find('timed out')==0:
					plain_client=client_t(self,new_sock,addr,uid)

					if sig_data:
						plain_client.add_recv_chunk(sig_data)

					self.clients[uid]=plain_client

				else:
					bad_ssl=True

			if not bad_ssl and self.onopen:
				self.onopen(self.clients[uid])

			if bad_ssl:
				self.remove(uid)

		for uid in self.clients.keys():
			if self.clients[uid].available() and not self.clients[uid].recv():
				self.remove(uid)

	def send(self,uid,data):
		if uid in self.clients.keys():
			self.clients[uid].send(data)

	def kill(self,uid):
		if uid in self.clients.keys():
			self.clients[uid].close()
			self.remove(uid)

	def remove(self,uid):
		if uid in self.clients.keys():
			old_client=self.clients[uid]
			del self.clients[uid]
			if self.onclose:
				self.onclose(old_client)
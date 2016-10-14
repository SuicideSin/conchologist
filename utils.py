import select

def available(conn):
	try:
		readable,writeable,errored=select.select([conn],[],[],0)
		if conn in readable:
			return True
	except KeyboardInterrupt:
		exit(1)
	except:
		pass
	return False
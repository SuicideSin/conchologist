function terminal_manager_t(doorway_manager)
{
	if(!doorway_manager)
		return null;
	this.doorway_manager=doorway_manager;
	var _this=this;
	this.update();
	this.terminals={};
	this.updates={};
}

terminal_manager_t.prototype.destroy=function()
{
	if(this.terminals)
	{
		for(var key in this.terminals)
			this.terminals[key].destroy();
		this.terminals=null;
	}
	if(this.updates)
		this.updates=null;
	if(this.doorway_manager)
		this.doorway_manager=null;
}

terminal_manager_t.prototype.update=function()
{
	var _this=this;
	var xhr=new XMLHttpRequest();
	xhr.onreadystatechange=function()
	{
		if(xhr.readyState==4)
		{
			if(xhr.status==200)
			{
				var modified=[];
				try
				{
					var updates=JSON.parse(xhr.responseText);
					for(var key in updates.result)
					{
						if(!(key in _this.terminals))
						{
							_this.updates[key]=0;
							var doorway=_this.doorway_manager.add
							({
								title:key,
								minimized:true,
								min_size:
								{
									w:200,
									h:200
								},
								buttons:
								[
									{
										icon:'X',
										callback:function(){_this.kill(key);}
									},
									{
										icon:'[ ]',
										callback:function(){doorway.maximize();}
									},
									{
										icon:'-',
										callback:function(){doorway.set_minimized(true);}
									}
								]
							});
							var old_settings=localStorage.getItem(key);
							if(old_settings)
							{
								doorway.load(JSON.parse(old_settings));
							}
							else if(doorway&&Object.keys(_this.doorway_manager.doorways).length==1)
							{
								doorway.set_active(true);
							}
							_this.terminals[key]=new terminal_t(_this,key,doorway);
						}
						if(updates.result[key]&&updates.result[key].last_count>=_this.updates[key])
						{
							_this.updates[key]+=updates.result[key].new_lines.length;
							for(var line in updates.result[key].new_lines)
								_this.terminals[key].add_line(updates.result[key].new_lines[line]);
							modified.push(key);
						}
					}
					for(var key in _this.terminals)
					{
						var found=false;
						for(var ii=0;ii<modified.length;++ii)
							if(modified[ii]==_this.terminals[key].address)
							{
								found=true;
								break;
							}
						if(!found)
						{
							_this.doorway_manager.set_title(key,'Killed - '+key);
							_this.terminals[key].killed();
						}
					}
					_this.update();
				}
				catch(error)
				{
					console.log(error);
					setTimeout(function()
					{
						_this.update();
					},1000);
				}
			}
			else
			{
				setTimeout(function()
				{
					_this.update();
				},1000);
			}
		}
	};
	xhr.open("POST","",true);
	var data={method:"updates",params:this.updates};
	xhr.send(JSON.stringify(data));
}

terminal_manager_t.prototype.send=function(address,line)
{
	var xhr=new XMLHttpRequest();
	xhr.open("POST","",true);
	var data={method:"write",params:{address:address,line:line}};
	xhr.send(JSON.stringify(data));
}

terminal_manager_t.prototype.kill=function(address)
{
	var new_terminals={};
	for(var key in this.terminals)
		if(this.terminals[key].address!=address)
		{
			new_terminals[key]=this.terminals[key];
		}
		else
		{
			var xhr=new XMLHttpRequest();
			xhr.open("POST","",true);
			var data={method:"kill",params:{address:address}};
			xhr.send(JSON.stringify(data));
			this.terminals[key].destroy();
		}
	this.terminals=new_terminals;
}

function terminal_t(manager,address,doorway)
{
	if(!manager||!address||!doorway)
		return null;
		this.manager=manager;
	this.address=address;
	this.doorway=doorway;
	var _this=this;
	this.history_lookup=[];
	this.history_ptr=-1;

	this.el=document.createElement("div");
	this.doorway.win.appendChild(this.el);
	this.el.className="doorways terminal win";

	this.history=document.createElement("div");
	this.el.appendChild(this.history);
	this.history.className="doorways terminal history active";

	this.input=document.createElement("input");
	this.el.appendChild(this.input);
	this.input.className="doorways terminal input";
	this.input.placeholder="Command";
	this.input.addEventListener("keydown",function(evt)
	{
		if(evt.keyCode==9)
		{
			evt.preventDefault();
		}
		else if(evt.keyCode==13)
		{
			_this.manager.send(_this.address,this.value+"\n");
			this.value="";
		}
		else if(evt.keyCode==38)
		{
			evt.preventDefault();
			if(_this.history_ptr>0&&_this.history_lookup.length>0)
			{
				--_this.history_ptr;
				this.value=_this.history_lookup[_this.history_ptr];
				_this.move_cursor_end();
			}
		}
		else if(evt.keyCode==40)
		{
			evt.preventDefault();
			if(_this.history_ptr+1<=_this.history_lookup.length&&_this.history_lookup.length>0)
			{
				++_this.history_ptr;
				if(_this.history_ptr==_this.history_lookup.length)
				{
					this.value="";
				}
				else
				{
					this.value=_this.history_lookup[_this.history_ptr];
				}
				_this.move_cursor_end();
			}
		}
	});
	this.interval=setInterval(function()
	{
		localStorage.setItem(_this.address,JSON.stringify(_this.doorway.save()));
	},500);
}

terminal_t.prototype.add_line=function(line)
{
	var current_scroll=this.history.scrollHeight-this.history.scrollTop;
	var scroll_end=(Math.abs(current_scroll-this.history.offsetHeight)<20);
	var _this=this;
	//if(line.substr(0,2)!="> ")
	{
		this.history.appendChild(document.createTextNode(line));
		if(line.length>0&&line[line.length-1]=='\n')
			this.history.appendChild(document.createElement("br"));
	}
	if(scroll_end)
		setTimeout(function()
		{
			_this.history.scrollTop=_this.history.scrollHeight+1000;
		},100);
	if(line.substr(0,2)=="> "&&(this.history_lookup.length==0||
		this.history_lookup[this.history_lookup.length-1]!=line.substr(2,line.length)))
		this.history_lookup.push(line.substr(2,line.length));
	this.history_ptr=this.history_lookup.length;
	if(this.history_ptr<0)
		this.history_ptr=0;
}

terminal_t.prototype.destroy=function()
{
	if(this.interval)
	{
		clearInterval(this.interval);
		this.interval=null;
	}
	if(this.manager&&this.doorway)
	{
		this.manager.doorway_manager.remove(this.doorway.title);
		this.manager=this.address=this.doorway=this.el=null;
	}
	this.history_lookup=this.history_ptr=null;
}

terminal_t.prototype.move_cursor_end=function()
{
	var input=this.input;
	setTimeout(function()
	{
		input.selectionStart=input.selectionEnd=input.value.length;
	},0);
}

terminal_t.prototype.killed=function()
{
	this.history.className="doorways terminal history inactive";
	this.input.disabled=true;
}
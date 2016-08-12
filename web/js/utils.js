//Utils wrapper class.
function utils_t()
{}

//To get things like style.width and .offsetHeight.
//  (AKA things which are sometimes null and have "px" at the end).
utils_t.prototype.numify=function(value)
{
	var num=parseFloat(value);
	if(!num)
		num=0;
	return num;
}

//Makes a copy of an object.
utils_t.prototype.copy=function(obj)
{
	return JSON.parse(JSON.stringify(obj));
}

//Gets position of a single position event (mouse or touch).
utils_t.prototype.get_event_pos=function(event)
{
	var pos=null;
	if(event.touches&&event.touches.length==1)
		pos={x:event.touches[0].pageX,y:event.touches[0].pageY};
	else
		pos={x:event.pageX,y:event.pageY};
	return pos;
}

//Get element position (uses offsetLeft and offsetTop).
utils_t.prototype.get_el_pos=function(el)
{
	var pos=
	{
		x:null,
		y:null
	};
	if(el)
	{
		pos.x=this.numify(el.offsetLeft);
		pos.y=this.numify(el.offsetTop);
	}
	return pos;
}

//Get element size (uses offsetWidth and offsetHeight).
utils_t.prototype.get_el_size=function(el)
{
	var size=
	{
		w:null,
		h:null
	};
	if(el)
	{
		size.w=this.numify(el.offsetWidth);
		size.h=this.numify(el.offsetHeight);
	}
	return size;
}

//Set an event listener for an object.
//  Note, if el is not a DOM object, you need to make a map of arrays called event_listeners:
//    Example: this.event_listeners={click:[]};
//  Note, throws error on invalid listener.
//  Note, listeners will only be added ONCE.
utils_t.prototype.setEventListener=function(el,listener,callback)
{
	//Check listener...
	var found=false;
	for(var key in el.event_listeners)
		if(key==listener)
		{
			found=true;
			break;
		}
	if(!found)
		throw "Invalid event listener \""+listener+"\".";

	//Check and add callback...
	var index=el.event_listeners[listener].indexOf(callback);
	if(index<0)
		el.event_listeners[listener].push(callback);
}

//Remove an event listener from an object.
//  Note, throws error on invalid listener.
//  Note, throws error on invalid callback for listener.
utils_t.prototype.unsetEventListener=function(el,listener,callback)
{
	//Check listener...
	var found=false;
	for(var key in el.event_listeners)
		if(key==listener)
		{
			found=true;
			break;
		}
	if(!found)
		throw "Invalid event listener \""+listener+"\".";

	//Check and remove callback...
	var index=el.event_listeners[listener].indexOf(callback);
	if(index<0)
		throw "Invalid callback for \""+listener+"\".";
	el.event_listeners[listener].splice(index,1);
}

//Global utils object.
var utils=new utils_t();

#wrapper 
{
	width: 100%;
	min-width: 300px;
	max-width: 700px;
	margin-top: 0px;
	margin-left:0; margin-right:auto;
}

#topheader
{
	position: absolute;
	left: 0px;
	top: 0px;
	width: 100%;
	height: 60px;
	background-color: #EEEEEE;
	color: #666666;
	font-size: 18px;
	font-family: Arial, Helvetica, sans-serif;
}


#footer
{
	position: absolute;
	left: 0px;
	bottom: 1px;
	width: 100%;
	height: 60px;
	background-color: #EEEEEE;
	color: #666666;
	font-size: 18px;
	font-family: Arial, Helvetica, sans-serif;
}



#h1
{
	position: absolute;
	left: 0px;
	top: 0px;
	margin-left: 10px;
	color: #666666;
	font-size: 28px;
}

.b1
{
 margin-top: 2px;
 margin-left: 2px;
margin-bottom: 2px;
  -webkit-border-radius: 4;
  -moz-border-radius: 4;
  border-radius: 4px;
  font-family: Arial;
  color: #ffffff;
  font-size: 12px;
  background: #616161;
  padding: 5px 10px 5px 10px;
  text-decoration: none;
}

.b1:hover {
  background: #00c4ff;
  text-decoration: none;
}

.tableall
{
	border-collapse: collapse;
	width: 100%;
	border: 1px solid #ddd;
	font-size: 16px;
	background: #ffffff;
	font-family: Arial, Helvetica, sans-serif;
}
.tableleft
{
	text-align: right;
	padding: 12px;
	width:30%;
	
	font-family:Arial, sans-serif;font-size:12px;font-weight:bold;line-height:1.3em;color:#8e0000;padding:1.5px;
}
.tableright
{
	text-align: left;
	padding: 12px;
	width:60%;
}


.tablinks
{
	font-family:Helvetica, sans-serif;font-size:12px;font-weight:bold;line-height:1.3em;color:#000033;padding:1.5px;
}




ul.tab {
	list-style-type: none;
	margin: 0;
	padding: 0;
	overflow: hidden;
	border: 1px solid #ccc;
	background-color: #DADADA;
	border-top-left-radius: 6px;
	border-top-right-radius: 6px;
}

ul.tab li {float: left;}

ul.tab li a {
	display: inline-block;
	color: black;
	text-align: center;
	padding: 14px 16px;
	text-decoration: none;
	border-right: 1px solid #ccc;
	transition: 0.3s;
	font-size: 14px;
}

ul.tab li a:hover {
	background-color: #ddd;
}

ul.tab li a:focus, .active {
	background-color: #EEEEEE;
}

.tabcontent {
	display: none;
	padding: 16px 12px;
	background: #EEEEEE;
	border: 1px solid #ccc;
	border-top: none;
	
	border-bottom-left-radius: 6px;
	border-bottom-right-radius: 6px;
}

.tabcontent {
-webkit-animation: fadeEffect 0.5s;
	animation: fadeEffect 0.5s;
}

@-webkit-keyframes fadeEffect {
	from {opacity: 0;}
	to {opacity: 1;}
}

@keyframes fadeEffect {
	from {opacity: 0;}
	to {opacity: 1;}
}

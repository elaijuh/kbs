<?php
require("pcfuncs.php");
$userid = addslashes($_GET["id"]);
$uid = (int)($_GET["id"]);

$link = pc_db_connect();
$pc = pc_load_infor($link,$userid,$uid);
if(!$pc)
{
	pc_db_close($link);
	html_init("gb2312",$pcconfig["BBSNAME"]."Blog");		
	html_error_quit("对不起，您要查看的Blog不存在");
	exit();
}

$query = "SELECT css FROM userstyle WHERE uid = ".$pc["UID"]." LIMIT 0 , 1;";
$result = mysql_query($query,$link);
$rows = mysql_fetch_array($result);
mysql_free_result($result);
pc_db_close($link);

if(!$rows)
{
	html_init("gb2312",$pcconfig["BBSNAME"]."Blog");		
	html_error_quit("对不起，您要查看的用户尚未设定自定义CSS文件");
	exit();
}

header("Content-Type: text/css");
header("Content-Disposition: inline;filename=SMTHBlog_".$pc["USER"].".css");
echo stripslashes($rows[css]);
?>
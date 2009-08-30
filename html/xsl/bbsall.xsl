<?xml version='1.0' encoding='gb2312'?>
<xsl:stylesheet version='1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform' xmlns='http://www.w3.org/1999/xhtml'>
	<xsl:import href='misc.xsl'/>
	<xsl:output method='xml' encoding='gb2312' doctype-public='-//W3C//DTD HTML 4.01//EN' doctype-system='http://www.w3.org/TR/html4/strict.dtd' />
	<xsl:template match='bbsall'>
		<html>
			<head>
				<title>全部讨论区<xsl:call-template name='bbsname' /></title>
				<meta http-equiv='content-type' content='text/html; charset=gb2312' />
				<link rel='stylesheet' type='text/css' href='/css/bbs.css' />
			</head>
			<body><div class='main'>
				<xsl:call-template name='navgation-bar'><xsl:with-param name='perm' select='@p' /></xsl:call-template>
				<h3>[讨论区数: <xsl:value-of select="count(brd)" />]</h3>
				<table>
					<col class='no' /><col class='title' /><col class='cate' /><col class='desc' /><col class='bm' />
					<tr><th>序号</th><th>讨论区名称</th><th>类别</th><th>中文描述</th><th>版主</th></tr>
					<xsl:for-each select='brd'>
					<xsl:sort select="@title" />
					<tr>
						<xsl:attribute name='class'>
							<xsl:if test='position() mod 2 = 1'>light</xsl:if>
							<xsl:if test='position() mod 2 = 0'>dark</xsl:if>
						</xsl:attribute>
						<td><xsl:value-of select='position()' /></td>
						<td><a class='title'><xsl:choose>
							<xsl:when test='@dir="1"'><xsl:attribute name='href'>boa?board=<xsl:value-of select='@title' /></xsl:attribute>[ <xsl:value-of select='@title' /> ]</xsl:when>
							<xsl:otherwise><xsl:attribute name='href'>doc?board=<xsl:value-of select='@title' /></xsl:attribute><xsl:value-of select='@title' /></xsl:otherwise>
						</xsl:choose></a></td>
						<td><xsl:choose>
							<xsl:when test='@dir="1"'>[目录]</xsl:when>
							<xsl:otherwise><xsl:value-of select='@cate' /></xsl:otherwise>
						</xsl:choose></td>
						<td><a class='desc'><xsl:choose>
							<xsl:when test='@dir="1"'><xsl:attribute name='href'>boa?board=<xsl:value-of select='@title' /></xsl:attribute><xsl:value-of select='@desc' /></xsl:when>
							<xsl:otherwise><xsl:attribute name='href'>doc?board=<xsl:value-of select='@title' /></xsl:attribute><xsl:value-of select='@desc' /></xsl:otherwise>
						</xsl:choose></a></td>
						<td>
							<xsl:call-template name='splitbm'>
								<xsl:with-param name='names' select='@bm' />
								<xsl:with-param name='isdir' select='@dir' />
								<xsl:with-param name='isfirst' select='1' />
							</xsl:call-template>
						</td>
					</tr>
				</xsl:for-each>
				</table>
			</div></body>
		</html>
	</xsl:template>
</xsl:stylesheet>

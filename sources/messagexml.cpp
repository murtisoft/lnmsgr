/****************************************************************************
**
** This file is part of LAN Messenger.
**
** Copyright (c) LAN Messenger Contributors.
** 
** LAN Messenger is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** LAN Messenger is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with LAN Messenger.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/


#include "messagexml.h"

MessageXml::MessageXml(void) : QDomDocument() {
	QDomElement root = createElement(XN_ROOT);
	appendChild(root);
	
	QDomElement head = createElement(XN_HEAD);
	root.appendChild(head);

	QDomElement body = createElement(XN_BODY);
	root.appendChild(body);
}

MessageXml::MessageXml(const QString& text) : QDomDocument() {
	setContent(text);
}

MessageXml::~MessageXml(void) {
}

bool MessageXml::addHeader(const QString& nodeName, const QString& nodeValue) {
	return addXmlNode(XN_HEAD, nodeName, nodeValue);
}

bool MessageXml::addData(const QString& nodeName, const QString& nodeValue) {
	return addXmlNode(XN_BODY, nodeName, nodeValue);
}

QString MessageXml::header(const QString& nodeName) {
	return getXmlNode(XN_HEAD, nodeName);
}

QString MessageXml::data(const QString& nodeName) {
	return getXmlNode(XN_BODY, nodeName);
}

bool MessageXml::removeHeader(const QString& nodeName) {
	return removeXmlNode(XN_HEAD, nodeName);
}

bool MessageXml::removeData(const QString& nodeName) {
	return removeXmlNode(XN_BODY, nodeName);
}

bool MessageXml::headerExists(const QString& nodeName) {
	return xmlNodeExists(XN_HEAD, nodeName);
}

bool MessageXml::dataExists(const QString& nodeName) {
	return xmlNodeExists(XN_BODY, nodeName);
}

MessageXml MessageXml::clone(void) {
	MessageXml newMsg;
	newMsg.setContent(toString());
	return newMsg;
}

bool MessageXml::isValid(void) {
	QDomElement root = documentElement();
	if(root.isNull())
		return false;

	if(root.tagName() == APP_MARKER)
		return true;
	
	return false;
}

bool MessageXml::addXmlNode(const QString& parentNode, const QString& nodeName, const QString& nodeValue) {
	QDomElement root = documentElement();
	if(root.isNull())
		return false;

	QDomNodeList nodes = root.elementsByTagName(parentNode);
	if(nodes.isEmpty())
		return false;

	QDomElement parent = nodes.at(0).toElement();
	QDomElement element = createElement(nodeName);
	parent.appendChild(element);
	QDomText elementText = createTextNode(nodeValue);
	element.appendChild(elementText);
	return true;
}

QString MessageXml::getXmlNode(const QString& parentNode, const QString& nodeName) {
	QDomElement root = documentElement();
	if(root.isNull())
		return QString();

	QDomNodeList nodes = root.elementsByTagName(parentNode);
	if(nodes.isEmpty())
		return QString();

	QDomElement parent = nodes.at(0).toElement();
	nodes = parent.elementsByTagName(nodeName);
	if(nodes.isEmpty())
		return QString();

	QDomElement element = nodes.at(0).toElement();
	return element.text();
}

bool MessageXml::removeXmlNode(const QString& parentNode, const QString& nodeName) {
	QDomElement root = documentElement();
	if(root.isNull())
		return false;

	QDomNodeList nodes = root.elementsByTagName(parentNode);
	if(nodes.isEmpty())
		return false;

	QDomElement parent = nodes.at(0).toElement();
	nodes = parent.elementsByTagName(nodeName);
	if(nodes.isEmpty())
		return false;

	QDomElement element = nodes.at(0).toElement();
	QDomNode newNode = parent.removeChild(element);
	if(newNode.isNull())
		return false;

	return true;
}

bool MessageXml::xmlNodeExists(const QString& parentNode, const QString& nodeName) {
	QDomElement root = documentElement();
	if(root.isNull())
		return false;

	QDomNodeList nodes = root.elementsByTagName(parentNode);
	if(nodes.isEmpty())
		return false;

	QDomElement parent = nodes.at(0).toElement();
	nodes = parent.elementsByTagName(nodeName);
	if(nodes.isEmpty())
		return false;

	return true;
}
#!env python
#tmx2txt map converter

import sys
import xml.dom.minidom as xmldom

if len(sys.argv) != 3:
	print("Usage:\n\ttmx2txt <input.tmx> <output.txt>")
	exit(1);

xmlMap = xmldom.parse(sys.argv[1])

#get tile table 
xmlMapData = xmlMap.getElementsByTagName('data')[0]

#get tilesets first gids in tile table
xmlMapTilesets = xmlMap.getElementsByTagName('tileset')
tilesetsFirstGIDs = dict() 
for tileset in xmlMapTilesets:
	if(tileset.getAttribute('name') != 'objects'):
		tilesetsFirstGIDs[int(tileset.getAttribute('firstgid'))]=tileset.getAttribute('name')

#only one tileset per map
txtMapData = []
txtMapTileset = ''
maxGid = 0
for tile in xmlMapData.getElementsByTagName('tile'):
	if txtMapTileset == '':
		for k in sorted(tilesetsFirstGIDs.keys()):
			if int(tile.getAttribute('gid')) >= k:
				maxGid = k
		txtMapTileset = tilesetsFirstGIDs[maxGid]
	txtMapData.append(int(tile.getAttribute('gid'))-maxGid)
	
mapWidth = int(xmlMap.getElementsByTagName('map')[0].getAttribute('width'))
mapHeight= int(xmlMap.getElementsByTagName('map')[0].getAttribute('height'))
mapTilesize = int(xmlMap.getElementsByTagName('map')[0].getAttribute('tilewidth'))

print('Map size: ' + '{0}*{2} x {1}*{2}'.format(mapWidth,mapHeight,mapTilesize));
print('Map tileset: ' + txtMapTileset)

txtMapObjects = []

#reading objects
xmlMapObjects = (xmlMap.getElementsByTagName('objectgroup')[0]).getElementsByTagName('object');
for mapObject in xmlMapObjects:
	objType = mapObject.getAttribute('type');
	posX = int(int(mapObject.getAttribute('x'))/mapTilesize)
	posY = int(int(mapObject.getAttribute('y'))/mapTilesize) - 1

	if objType == 'spawn':
		txtMapObjects.append([objType,posX,posY,None,None])
	elif objType == 'teleport':
		mapObjProps = mapObject.getElementsByTagName('property')
		tpID = -1
		tpDestID = -1
		for mapObjProp in mapObjProps:
			propName = mapObjProp.getAttribute('name')
			propValue = mapObjProp.getAttribute('value')
			if propName == 'id': tpID = int(propValue)
			elif propName == 'dest_id': tpDestID = propValue

		txtMapObjects.append([objType,posX,posY,tpID,tpDestID])
	elif objType == 'sign':
		signText = mapObject.getElementsByTagName('property')[0].getAttribute('value')
		txtMapObjects.append([objType,posX,posY,"\""+signText+"\"",None])
	elif objType == 'item':
		itemLevel = 0
		itemID = 0
		mapObjProps = mapObject.getElementsByTagName('property')
		for mapObjProp in mapObjProps:
			propName = mapObjProp.getAttribute('name')
			propValue = mapObjProp.getAttribute('value')
			
			if propName == 'item_level': itemLevel = int(propValue)
			elif propName == 'item_id': itemID = int(propValue)

		txtMapObjects.append([objType,posX,posY,itemLevel,itemID])
	elif objType == 'shop':
		txtMapObjects.append([objType,posX,posY,None,None])
	elif objType == 'monster':
		mapObjProps = mapObject.getElementsByTagName('property')
		monsterLevel = 1
		monsterType = 'snake'
		for mapObjProp in mapObjProps:
			propName = mapObjProp.getAttribute('name')
			propValue = mapObjProp.getAttribute('value')
			if propName == 'type': monsterType = propValue
			elif propName == 'level': monsterLevel = int(propValue)
		txtMapObjects.append([objType,posX,posY,monsterType,monsterLevel])

fout = open(sys.argv[2],'w')
fout.write('| begin\n| input source: ' + sys.argv[1] + '\n')
fout.write('#tileset ' + txtMapTileset + '\n')
fout.write('#data ')
for t in txtMapData: fout.write(str(t))
fout.write('\n')

for txtMapObj in txtMapObjects:
	fout.write('#{} {} {} '.format(txtMapObj[0],txtMapObj[1],txtMapObj[2]))
	if txtMapObj[3] != None: fout.write(str(txtMapObj[3]))
	if txtMapObj[4] != None: fout.write(' ' + str(txtMapObj[4]))
	fout.write('\n')

fout.write('| end\n')
fout.close()

print('Done')

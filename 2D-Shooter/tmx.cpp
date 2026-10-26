#include "tmx.h"

#include <tinyxml2.h>
#include <filesystem>
#include <sstream>
#include <string_view>

using namespace tmx;

std::unique_ptr<Map> tmx::loadMap(const std::string& mapFilePath)
{
	using namespace tinyxml2;
	std::filesystem::path mapPath(mapFilePath);

	tmx::Map* map = nullptr;

	XMLDocument doc;
	doc.LoadFile(mapPath.string().c_str());
	XMLElement* mapDoc = doc.FirstChildElement("map");

	if (!mapDoc) return nullptr;
	
	map = new tmx::Map;
	map->mapWidth = mapDoc->IntAttribute("width");
	map->mapHeight = mapDoc->IntAttribute("height");
	map->tileWidth = mapDoc->IntAttribute("tilewidth");
	map->tileHeight = mapDoc->IntAttribute("tileheight");

	for (XMLElement* child = mapDoc->FirstChildElement(); child != nullptr; child = child->NextSiblingElement())
	{
		if (std::string_view(child->Name()) == "tileset")
		{
			int firstgid = child->IntAttribute("firstgid");

			// load tileset XML
			XMLDocument tilesetDoc;
			auto sourcePath = mapPath.parent_path().append(child->Attribute("source"));
			tilesetDoc.LoadFile(sourcePath.string().c_str());

			XMLElement* ts = tilesetDoc.FirstChildElement("tileset");
			if (ts)
			{
				int tileWidth = ts->IntAttribute("tilewidth");
				int tileHeight = ts->IntAttribute("tileheight");
				int count = ts->IntAttribute("tilecount");
				int columns = ts->IntAttribute("columns");
				tmx::TileSet newTileSet(firstgid, count, tileWidth, tileHeight, columns);

				for (XMLElement* tile = ts->FirstChildElement("tile"); tile != nullptr; tile = tile->NextSiblingElement("tile"))
				{
					Tile newTile;
					newTile.id = tile->IntAttribute("id");

					XMLElement* image = tile->FirstChildElement("image");
					if (image)
					{
						newTile.image.source = image->Attribute("source");
						newTile.image.width = image->IntAttribute("width");
						newTile.image.height = image->IntAttribute("height");
					}
					newTileSet.tiles.push_back(std::move(newTile));
				}
				map->tileSets.push_back(std::move(newTileSet));
			}
		}
		else if (std::string_view(child->Name()) == "layer")
		{
			tmx::Layer layer;
			layer.id = child->IntAttribute("id");
			layer.name = child->Attribute("name");
			layer.data.reserve(map->mapWidth * map->mapHeight);

			XMLElement* data = child->FirstChildElement("data");
			std::stringstream dataStream(data->GetText());
			for (int i; dataStream >> i;)
			{
				layer.data.push_back(i);
				if (dataStream.peek() == ',') dataStream.ignore();
			}
			map->layers.push_back(std::move(layer));
		}
		else if (std::string_view(child->Name()) == "objectgroup")
		{
			tmx::ObjectGroup objGroup;
			objGroup.name = child->Attribute("name");
			objGroup.id = child->IntAttribute("id");

			for (XMLElement* obj = child->FirstChildElement("object"); obj != nullptr; obj = obj->NextSiblingElement("object"))
			{
				tmx::LayerObject newObj;

				const char* attrname = obj->Attribute("name");
				if (attrname) newObj.name = attrname;

				const char* attrType = obj->Attribute("type");
				if (attrType) newObj.type = attrType;

				newObj.id = obj->IntAttribute("id");
				newObj.x = obj->FloatAttribute("x");
				newObj.y = obj->FloatAttribute("y");

				objGroup.objects.push_back(std::move(newObj));
			}

			map->layers.push_back(std::move(objGroup));
		}
	}

	return std::unique_ptr<tmx::Map>(map);
}
 
#pragma once

#include "ImageFile.h"
#include "MipMapGenerator.h"
#include "TileCache.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <unordered_map>
#include <set>

class ImageViewer {
public:
	explicit ImageViewer(const std::string& filename);
	~ImageViewer() = default;
    
	void Run();
    
	void DrawPixel(int32_t x, int32_t y, sf::Color color);
	void DrawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, sf::Color color);
    
private:
	std::unique_ptr<ImageFile> m_imageFile;
	TileCache<TileKey, boost::interprocess::mapped_region> m_tileCache;
    
	float m_zoom;
	float m_offsetX;
	float m_offsetY;
	sf::Vector2f m_dragStart;
	bool m_dragging;
    
	// SFML
	sf::RenderWindow m_window;
	sf::RenderTexture m_renderTexture;
	sf::Sprite m_sprite;
	sf::Clock m_clock;
    
	void HandleEvents();
	void Update();
	void Render();
    
	uint32_t GetCurrentMipLevel() const;
    
	std::set<TileKey> GetVisibleTiles() const;
    
	void LoadVisibleTiles();
    
	void RenderTiles();
    
	sf::Vector2f ScreenToImage(sf::Vector2f screenPos) const;
    
	std::pair<int32_t, int32_t> ImageToTile(float x, float y, uint32_t level) const;
};
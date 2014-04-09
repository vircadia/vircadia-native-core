//
//  Renderer.h
//  interface/src/starfield/renderer
//
//  Created by Tobias Schwinger on 3/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

namespace starfield {

    class Renderer {
    public:

        Renderer(InputVertices const& src, unsigned numStars, unsigned tileResolution);
        ~Renderer();
        void render(float perspective, float aspect, mat4 const& orientation, float alpha);
        
    private:
        // renderer construction

        void prepareVertexData(InputVertices const& vertices, unsigned numStars, Tiling const& tiling);

        // FOV culling / LOD

        class TileSelection;
        friend class Renderer::TileSelection;

        class TileSelection {

            public:
                struct Cursor { Tile* current, * firstInRow; };
            
            private:
                Renderer& _rendererRef;
                Cursor* const _stackArray;
                Cursor* _stackPos;
                Tile const* const _tileArray;
                Tile const* const _tilesEnd;

            public:
                TileSelection(Renderer& renderer, Tile const* tiles, Tile const* tiles_end, Cursor* stack) :
                    _rendererRef(renderer),
                    _stackArray(stack),
                    _stackPos(stack),
                    _tileArray(tiles),
                    _tilesEnd(tiles_end) { }
     
            protected:
                bool select(Cursor const& cursor);
                bool process(Cursor const& cursor);
                void right(Cursor& cursor) const;
                void left(Cursor& cursor) const;
                void up(Cursor& cursor) const;
                void down(Cursor& cursor) const;
                void defer(Cursor const& cursor);
                bool deferred(Cursor& cursor);
        };

        bool visitTile(Tile* tile);
        bool isTileVisible(unsigned index);
        unsigned prepareBatch(unsigned const* indices, unsigned const* indicesEnd);
        
        // GL API handling 

        void glAlloc();
        void glFree();
        void glUpload(GLsizei numStars);
        void glBatch(GLfloat const* matrix, GLsizei n_ranges, float alpha);
        
        // variables

        GpuVertex* _dataArray;
        Tile* _tileArray;
        GLint* _batchOffs;
        GLsizei* _batchCountArray;
        GLuint _vertexArrayHandle;
        ProgramObject _program;
        int _alphaLocationHandle;

        Tiling _tiling;

        unsigned* _outIndexPos;
        vec3 _wRowVec;
        float _halfPerspectiveAngle;

    };

}

#endif


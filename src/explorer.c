#include <assert.h>
#include <stdint.h>

#include <zlib.h>
#include <raylib.h>

#define RAYGUI_IMPLEMENTATION
#include "vendor/raygui.h"
#include "vendor/style_dark.h"
#include "vendor/mkdir_p.h"
#include "vendor/stb_ds.h"

#include "config.h"
#include "filetree.h"
#include "rf.h"

#define PANEL_PADDING 8
#define HEADER_HEIGHT 24

static int g_screenWidth = 640;
static int g_screenHeight = 360;

Vector2 panelScroll;

size_t numExpandedLines(filetree_node_t *node)
{
    size_t len = 1;

    if (node->expanded) {
        size_t numChildren = stbds_arrlenu(node->children);
        for (int i = 0; i < numChildren; ++i) {
            len += numExpandedLines(node->children[i]);
        }
    }

    return len;
}

filetree_node_t* getPackingRoot(filetree_node_t *node)
{
    filetree_node_t *r = node;

    do {
        if (!(r->res->flags & RES_FLAG_NO_LOC)) {
            return r;
        }
    } while ((r = r->parent));

    return NULL;
}

static filetree_node_t* ui_ctxMenuTarget = NULL;
static Vector2 ui_ctxMenuPos;

// FIXME: does not support base file (dt/ls)
// FIXME: does not support localized files (data(us_en))
void extractNodeToFile(filetree_node_t *node)
{
    if (node->res->flags & RES_FLAG_DIR) {
        size_t numChildren = stbds_arrlenu(node->children);
        for (int i = 0; i < numChildren; ++i) {
            extractNodeToFile(node->children[i]);
        }
        return;
    }

    filetree_node_t *pr = getPackingRoot(node);
    assert(pr);
    const char *localFilename = TextFormat("%sdata/%spacked", UPDATE_CONTENT_PATH, pr->path);

    if (!FileExists(localFilename)) {
        printf("%s does not exist; skipping %s...\n", localFilename, node->path);
        return;
    }

    uint8_t *rawFileData = (uint8_t*)malloc(node->res->sizeCompressed);

    FILE *fIn = fopen(localFilename, "rb");
    fseek(fIn, node->res->packOffset, SEEK_SET);
    fread(rawFileData, node->res->sizeCompressed, 1, fIn);
    fclose(fIn);

    const char *path = TextFormat("%sdata/%s", EXTRACT_PATH, node->path);
    printf(">>> extracting %s\n", path);

    mkdir_p(GetDirectoryPath(path));
    FILE *fOut = fopen(path, "wb");
    size_t dataOffset = 0;

    // not compressed
    if (node->res->sizeCompressed == node->res->sizeUncompressed) {
        dataOffset = 0x80;
    }

    {
        uLongf destLen = node->res->sizeUncompressed;
        uint8_t *uncompressedFileData = (uint8_t*)calloc(1, destLen);

        uncompress(
            uncompressedFileData, &destLen,
            rawFileData+dataOffset, node->res->sizeCompressed-dataOffset
        );

        fwrite(uncompressedFileData, destLen, 1, fOut);
        free(uncompressedFileData);
    }

    fclose(fOut);

    free(rawFileData);
}

void drawFileNode(filetree_node_t *node, int x, int y, int *currentLineIdx, int startI, int endI)
{
    resource_t *res = node->res;

    if (res) {
        if (*currentLineIdx >= startI && *currentLineIdx < endI) {
            int depth = res->flags & 0xff;
            int x2 = x + depth * 18;
            int y2 = y - 8 + *currentLineIdx * 18;

            const char *icon;

            if (res->flags & RES_FLAG_DIR) {
                if (node->expanded) {
                    icon = "#1#";
                } else {
                    icon = "#217#";
                }
            } else if (res->flags & RES_FLAG_OVERRIDE) {
                icon = "#8#";
            } else {
                icon = "#218#";
            }

            const char *s;
            
            size_t numChildren = stbds_arrlenu(node->children);
            if (numChildren > 0) {
                s = TextFormat("%s%s (%ld) - 0x%04X", icon, res->filename, numChildren, res->flags);
            } else {
                s = TextFormat("%s%s - 0x%04X", icon, res->filename, res->flags);
            }

            int oldColor = GuiGetStyle(LABEL, TEXT_COLOR_NORMAL);
            if (res->flags & RES_FLAG_OVERRIDE)
                GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, 0x8e67d6FF);
            else if (res->flags & RES_FLAG_NO_LOC)
                GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, 0x0067d6FF);

            Rectangle btnRect = { x2, y2, 640, 16 };
            int btnState = GuiFileLabelButton(btnRect, s);

            GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, oldColor);

            if (btnState == 2) {
                if (ui_ctxMenuTarget) {
                    ui_ctxMenuTarget = NULL;
                } else {
                    ui_ctxMenuTarget = node;
                    ui_ctxMenuPos = GetMousePosition();
                }

            } else if (btnState == 1) {
                if (res->flags & RES_FLAG_DIR) {
                    node->expanded = !node->expanded;
                }
            }
        }

        ++(*currentLineIdx);
    }

    if (node->expanded) {
        size_t numChildren = stbds_arrlenu(node->children);

        for (int childIdx = 0; childIdx < numChildren; ++childIdx) {
            drawFileNode(node->children[childIdx], x, y, currentLineIdx, startI, endI);
        }
    }
}

void drawContextMenu()
{
    GuiClearExclusive();
    Vector2 mousePoint = GetMousePosition();
    int numOptions = 2;

    float width = 150;
    float height = numOptions * 18 + 8 * 2;
    Rectangle ctxPanelRect = { ui_ctxMenuPos.x, ui_ctxMenuPos.y, width, height };
    float y = ctxPanelRect.y + 8;

    GuiPanel(ctxPanelRect, NULL);
    if (GuiLabelButton((Rectangle) { ctxPanelRect.x + 8, y, width - 16, 18 }, "Extract...")) {
        extractNodeToFile(ui_ctxMenuTarget);
        ui_ctxMenuTarget = NULL;
        GuiClearExclusive();
    }
    y += 18;


    int prevState = GuiGetState();
    GuiSetState(STATE_DISABLED);
    if (GuiLabelButton((Rectangle) { ctxPanelRect.x + 8, y, width - 16, 18 }, "Delete")) {
        printf("afawwfaf\n");
    }
    GuiSetState(prevState);

    GuiSetExclusive(ctxPanelRect);

    int buttonReleased = (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) || IsMouseButtonReleased(MOUSE_RIGHT_BUTTON));

    if (buttonReleased && !CheckCollisionPointRec(mousePoint, ctxPanelRect)) {
        ui_ctxMenuTarget = NULL;
        GuiClearExclusive();
    }
}

void startExplorerWindow(filetree_node_t *tree)
{
    tree->expanded = true;
    tree->children[0]->expanded = true;

    SetTraceLogLevel(LOG_WARNING);
    InitWindow(g_screenWidth, g_screenHeight, "resource(us_en)");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);
    GuiLoadStyleDark();

    while (!WindowShouldClose()) {
        if (IsWindowResized()) {
            g_screenWidth = GetScreenWidth();
            g_screenHeight = GetScreenHeight();
        }

        BeginDrawing();
        ClearBackground(BLACK);

        int x = -1;
        int y = -1;
        int w = g_screenWidth+2;
        int h = g_screenHeight+2;

        Rectangle view = { 0 };

        size_t numResources = numExpandedLines(tree);

        GuiScrollPanel(
            (Rectangle) { x, y, w, h },
            NULL,
            (Rectangle) { x+PANEL_PADDING, y+PANEL_PADDING, w-(PANEL_PADDING*2), numResources * 18 + (PANEL_PADDING*2) },
            &panelScroll,
            &view
        );

        BeginScissorMode(x, y + PANEL_PADDING, w, h - (PANEL_PADDING*2));
        int startI = fabs(floorf(panelScroll.y / 18));
        int endI = startI + (g_screenHeight / 18);

        if (endI > numResources-1)
            endI = numResources-1;

        x += panelScroll.x;
        y += panelScroll.y + 18;

        int i = 0;
        drawFileNode(tree, x, y, &i, startI, endI);
        EndScissorMode();

        // ----
        if (ui_ctxMenuTarget) {
            drawContextMenu();
        }

        EndDrawing();
    }
}

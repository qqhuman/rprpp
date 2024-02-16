#include "rpr_helper.h"

void ErrorManager(VkResult result, const char* fileName, int line)
{
    std::cerr << "ERROR detected - program will stop." << std::endl;
    std::cerr << "file = " << fileName << std::endl;
    std::cerr << "line = " << line << std::endl;
    std::cerr << "Vk result = " << result << std::endl;
    assert(0);
}

void ErrorManager(rpr_status errorCode, const char* fileName, int line)
{
    std::cerr << "ERROR detected - program will stop." << std::endl;
    std::cerr << "file = " << fileName << std::endl;
    std::cerr << "line = " << line << std::endl;
    std::cerr << "RPR error code = " << errorCode << std::endl;
    assert(0);
}

void ErrorManager(RprPpError errorCode, const char* fileName, int line)
{
    std::cerr << "ERROR detected - program will stop." << std::endl;
    std::cerr << "file = " << fileName << std::endl;
    std::cerr << "line = " << line << std::endl;
    std::cerr << "RPRPP error code = " << errorCode << std::endl;
    assert(0);
}

void CheckNoLeak(rpr_context context)
{
    rpr_int status = RPR_SUCCESS;

    std::vector<rpr_context_info> type;
    type.push_back(RPR_CONTEXT_LIST_CREATED_CAMERAS);
    type.push_back(RPR_CONTEXT_LIST_CREATED_MATERIALNODES);
    type.push_back(RPR_CONTEXT_LIST_CREATED_LIGHTS);
    type.push_back(RPR_CONTEXT_LIST_CREATED_SHAPES);
    type.push_back(RPR_CONTEXT_LIST_CREATED_POSTEFFECTS);
    type.push_back(RPR_CONTEXT_LIST_CREATED_HETEROVOLUMES);
    type.push_back(RPR_CONTEXT_LIST_CREATED_GRIDS);
    type.push_back(RPR_CONTEXT_LIST_CREATED_BUFFERS);
    type.push_back(RPR_CONTEXT_LIST_CREATED_IMAGES);
    type.push_back(RPR_CONTEXT_LIST_CREATED_FRAMEBUFFERS);
    type.push_back(RPR_CONTEXT_LIST_CREATED_SCENES);
    type.push_back(RPR_CONTEXT_LIST_CREATED_CURVES);
    type.push_back(RPR_CONTEXT_LIST_CREATED_MATERIALSYSTEM);
    type.push_back(RPR_CONTEXT_LIST_CREATED_COMPOSITE);
    type.push_back(RPR_CONTEXT_LIST_CREATED_LUT);

    std::vector<void*> listRemainingObjects;

    for (int iType = 0; iType < type.size(); iType++) {

        size_t sizeParam = 0;
        status = rprContextGetInfo(context, type[iType], 0, 0, &sizeParam);
        RPR_CHECK(status);

        unsigned int nbObject = sizeParam / sizeof(void*);

        if (nbObject > 0) {
            std::cout << "leak of " << nbObject;
            if (type[iType] == RPR_CONTEXT_LIST_CREATED_CAMERAS)
                std::cout << " cameras\n";
            else if (type[iType] == RPR_CONTEXT_LIST_CREATED_MATERIALNODES)
                std::cout << " material nodes\n";
            else if (type[iType] == RPR_CONTEXT_LIST_CREATED_LIGHTS)
                std::cout << " lights\n";
            else if (type[iType] == RPR_CONTEXT_LIST_CREATED_SHAPES)
                std::cout << " shapes\n";
            else if (type[iType] == RPR_CONTEXT_LIST_CREATED_POSTEFFECTS)
                std::cout << " postEffects\n";
            else if (type[iType] == RPR_CONTEXT_LIST_CREATED_HETEROVOLUMES)
                std::cout << " heteroVolumes\n";
            else if (type[iType] == RPR_CONTEXT_LIST_CREATED_GRIDS)
                std::cout << " grids\n";
            else if (type[iType] == RPR_CONTEXT_LIST_CREATED_BUFFERS)
                std::cout << " buffers\n";
            else if (type[iType] == RPR_CONTEXT_LIST_CREATED_IMAGES)
                std::cout << " images\n";
            else if (type[iType] == RPR_CONTEXT_LIST_CREATED_FRAMEBUFFERS)
                std::cout << " framebuffers\n";
            else if (type[iType] == RPR_CONTEXT_LIST_CREATED_SCENES)
                std::cout << " scenes\n";
            else if (type[iType] == RPR_CONTEXT_LIST_CREATED_CURVES)
                std::cout << " curves\n";
            else if (type[iType] == RPR_CONTEXT_LIST_CREATED_MATERIALSYSTEM)
                std::cout << " materialsystems\n";
            else if (type[iType] == RPR_CONTEXT_LIST_CREATED_COMPOSITE)
                std::cout << " composites\n";
            else if (type[iType] == RPR_CONTEXT_LIST_CREATED_LUT)
                std::cout << " luts\n";
            else {
                std::cout << " ???\n";
            }

            unsigned int idFirstTime = listRemainingObjects.size();
            listRemainingObjects.assign(idFirstTime + nbObject, nullptr);
            status = rprContextGetInfo(context, type[iType], sizeParam, &listRemainingObjects[idFirstTime], nullptr);
            RPR_CHECK(status);
        }
    }

    if (listRemainingObjects.size() != 0) {
        std::cout << "Warning : this context has some leak (" << listRemainingObjects.size() << " item(s))\n";
    }
}

rpr_shape ImportOBJ(const std::string& file, rpr_context ctx)
{
    rpr_int status = RPR_SUCCESS;

    std::ifstream infile(file.c_str());

    if (!infile.is_open() || infile.fail())
        return nullptr;

    std::string line;
    std::vector<rpr_float> pos;
    std::vector<rpr_float> normal;
    std::vector<rpr_float> texture;
    std::vector<rpr_int> face_pos;
    std::vector<rpr_int> face_normal;
    std::vector<rpr_int> face_texture;
    std::vector<rpr_int> face;

    while (std::getline(infile, line)) {
        // empty line
        if (line.size() < 2)
            continue;

        // comment
        if (line[0] == '#')
            continue;

        if (line.substr(0, 2) == "v ") {
            std::istringstream iss(line.substr(1));
            rpr_float x, y, z;
            if (!(iss >> x >> y >> z)) {
                break;
            } // error
            pos.push_back(x);
            pos.push_back(y);
            pos.push_back(z);
        }

        if (line.substr(0, 3) == "vn ") {
            std::istringstream iss(line.substr(2));
            rpr_float x, y, z;
            if (!(iss >> x >> y >> z)) {
                break;
            } // error
            normal.push_back(x);
            normal.push_back(y);
            normal.push_back(z);
        }

        if (line.substr(0, 3) == "vt ") {
            std::istringstream iss(line.substr(2));
            rpr_float x, y, z;
            if (!(iss >> x >> y >> z)) {
                break;
            } // error
            texture.push_back(x);
            texture.push_back(y);
        }

        if (line.substr(0, 2) == "f ") {
            rpr_int f01, f02, f03, f11, f12, f13, f21, f22, f23 = 0;
            std::istringstream iss(line.substr(1));
            int nb = sscanf(line.c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d", &f01, &f02, &f03, &f11, &f12, &f13, &f21, &f22, &f23);

            if (nb != 9)
                break; // error

            face_pos.push_back(f01 - 1);
            face_pos.push_back(f11 - 1);
            face_pos.push_back(f21 - 1);

            face_normal.push_back(f03 - 1);
            face_normal.push_back(f13 - 1);
            face_normal.push_back(f23 - 1);

            face_texture.push_back(f02 - 1);
            face_texture.push_back(f12 - 1);
            face_texture.push_back(f22 - 1);

            face.push_back(3);
        }
    }

    rpr_shape mesh = nullptr;
    RPR_CHECK(rprContextCreateMesh(ctx,
        (rpr_float const*)&pos[0], pos.size() / 3, 3 * sizeof(float),
        (rpr_float const*)&normal[0], normal.size() / 3, 3 * sizeof(float),
        (rpr_float const*)&texture[0], texture.size() / 2, 2 * sizeof(float),
        (rpr_int const*)&face_pos[0], sizeof(rpr_int),
        (rpr_int const*)&face_normal[0], sizeof(rpr_int),
        (rpr_int const*)&face_texture[0], sizeof(rpr_int),
        (rpr_int const*)&face[0], face.size(), &mesh));

    return mesh;
}
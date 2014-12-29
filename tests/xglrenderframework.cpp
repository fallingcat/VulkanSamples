/*
 * XGL Tests
 *
 * Copyright (C) 2014 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *   Courtney Goeltzenleuchter <courtney@lunarg.com>
 */

#include "xglrenderframework.h"

XglRenderFramework::XglRenderFramework() :
    m_cmdBuffer( XGL_NULL_HANDLE ),
    m_stateRaster( XGL_NULL_HANDLE ),
    m_colorBlend( XGL_NULL_HANDLE ),
    m_stateViewport( XGL_NULL_HANDLE ),
    m_stateDepthStencil( XGL_NULL_HANDLE ),
    m_stateMsaa( XGL_NULL_HANDLE ),
    m_width( 256.0 ),                   // default window width
    m_height( 256.0 )                   // default window height
{
    m_renderTargetCount = 1;

    m_render_target_fmt.channelFormat = XGL_CH_FMT_R8G8B8A8;
    m_render_target_fmt.numericFormat = XGL_NUM_FMT_UNORM;

    m_colorBindings[0].view = XGL_NULL_HANDLE;
    m_depthStencilBinding.view = XGL_NULL_HANDLE;
}

XglRenderFramework::~XglRenderFramework()
{

}

void XglRenderFramework::InitFramework()
{
    XGL_RESULT err;

    err = xglInitAndEnumerateGpus(&app_info, NULL,
                                  XGL_MAX_PHYSICAL_GPUS, &this->gpu_count, objs);
    ASSERT_XGL_SUCCESS(err);
    ASSERT_GE(this->gpu_count, 1) << "No GPU available";

    m_device = new XglDevice(0, objs[0]);
    m_device->get_device_queue();
}

void XglRenderFramework::ShutdownFramework()
{
    if (m_colorBlend) xglDestroyObject(m_colorBlend);
    if (m_stateMsaa) xglDestroyObject(m_stateMsaa);
    if (m_stateDepthStencil) xglDestroyObject(m_stateDepthStencil);
    if (m_stateRaster) xglDestroyObject(m_stateRaster);
    if (m_cmdBuffer) xglDestroyObject(m_cmdBuffer);

    if (m_stateViewport) {
        xglDestroyObject(m_stateViewport);
    }

    for (XGL_UINT i = 0; i < m_renderTargetCount; i++) {
        if (m_renderTargets[i]) {
        // TODO: XglImage should be able to destroy itself
//        m_renderTarget->
//        xglDestroyObject(*m_renderTarget);
        }
    }

    // reset the driver
    delete m_device;
    xglInitAndEnumerateGpus(&this->app_info, XGL_NULL_HANDLE, 0, &gpu_count, XGL_NULL_HANDLE);
}

void XglRenderFramework::InitState()
{
    XGL_RESULT err;

    m_render_target_fmt.channelFormat = XGL_CH_FMT_R8G8B8A8;
    m_render_target_fmt.numericFormat = XGL_NUM_FMT_UNORM;

    // create a raster state (solid, back-face culling)
    XGL_RASTER_STATE_CREATE_INFO raster = {};
    raster.sType = XGL_STRUCTURE_TYPE_RASTER_STATE_CREATE_INFO;
    raster.fillMode = XGL_FILL_SOLID;
    raster.cullMode = XGL_CULL_NONE;
    raster.frontFace = XGL_FRONT_FACE_CCW;
    err = xglCreateRasterState( device(), &raster, &m_stateRaster );
    ASSERT_XGL_SUCCESS(err);

    XGL_COLOR_BLEND_STATE_CREATE_INFO blend = {};
    blend.sType = XGL_STRUCTURE_TYPE_COLOR_BLEND_STATE_CREATE_INFO;
    err = xglCreateColorBlendState(device(), &blend, &m_colorBlend);
    ASSERT_XGL_SUCCESS( err );

    XGL_DEPTH_STENCIL_STATE_CREATE_INFO depthStencil = {};
    depthStencil.sType = XGL_STRUCTURE_TYPE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable      = XGL_FALSE;
    depthStencil.depthWriteEnable = XGL_FALSE;
    depthStencil.depthFunc = XGL_COMPARE_LESS_EQUAL;
    depthStencil.depthBoundsEnable = XGL_FALSE;
    depthStencil.minDepth = 0.f;
    depthStencil.maxDepth = 1.f;
    depthStencil.back.stencilDepthFailOp = XGL_STENCIL_OP_KEEP;
    depthStencil.back.stencilFailOp = XGL_STENCIL_OP_KEEP;
    depthStencil.back.stencilPassOp = XGL_STENCIL_OP_KEEP;
    depthStencil.back.stencilRef = 0x00;
    depthStencil.back.stencilFunc = XGL_COMPARE_ALWAYS;
    depthStencil.front = depthStencil.back;

    err = xglCreateDepthStencilState( device(), &depthStencil, &m_stateDepthStencil );
    ASSERT_XGL_SUCCESS( err );

    XGL_MSAA_STATE_CREATE_INFO msaa = {};
    msaa.sType = XGL_STRUCTURE_TYPE_MSAA_STATE_CREATE_INFO;
    msaa.sampleMask = 1;
    msaa.samples = 1;

    err = xglCreateMsaaState( device(), &msaa, &m_stateMsaa );
    ASSERT_XGL_SUCCESS( err );

    XGL_CMD_BUFFER_CREATE_INFO cmdInfo = {};

    cmdInfo.sType = XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO;
    cmdInfo.queueType = XGL_QUEUE_TYPE_GRAPHICS;
    err = xglCreateCommandBuffer(device(), &cmdInfo, &m_cmdBuffer);
    ASSERT_XGL_SUCCESS(err) << "xglCreateCommandBuffer failed";
}

void XglRenderFramework::InitViewport(float width, float height)
{
    XGL_RESULT err;

    XGL_VIEWPORT_STATE_CREATE_INFO viewport = {};
    viewport.viewportCount         = 1;
    viewport.scissorEnable         = XGL_FALSE;
    viewport.viewports[0].originX  = 0;
    viewport.viewports[0].originY  = 0;
    viewport.viewports[0].width    = 1.f * width;
    viewport.viewports[0].height   = 1.f * height;
    viewport.viewports[0].minDepth = 0.f;
    viewport.viewports[0].maxDepth = 1.f;

    err = xglCreateViewportState( device(), &viewport, &m_stateViewport );
    ASSERT_XGL_SUCCESS( err );
    m_width = width;
    m_height = height;
}

void XglRenderFramework::InitViewport()
{
    InitViewport(m_width, m_height);
}

void XglRenderFramework::InitRenderTarget()
{
    XGL_UINT i;

    for (i = 0; i < m_renderTargetCount; i++) {
        m_device->CreateImage(m_width, m_height, m_render_target_fmt,
                              XGL_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT |
                              XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                              &m_renderTargets[i]);
    }
}

void XglRenderFramework::GenerateBindRenderTargetCmd()
{
    XGL_UINT i;

    // bind render target
    for (i = 0; i < m_renderTargetCount; i++) {
        m_colorBindings[i].view  = m_renderTargets[i]->targetView();
        m_colorBindings[i].colorAttachmentState = XGL_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL;
    }
    if (m_depthStencilBinding.view) {
       xglCmdBindAttachments(m_cmdBuffer, m_renderTargetCount, m_colorBindings, &m_depthStencilBinding );
    } else {
       xglCmdBindAttachments(m_cmdBuffer, m_renderTargetCount, m_colorBindings, XGL_NULL_HANDLE );
    }
}

void XglRenderFramework::GenerateBindStateAndPipelineCmds()
{
    // set all states
    xglCmdBindStateObject( m_cmdBuffer, XGL_STATE_BIND_RASTER, m_stateRaster );
    xglCmdBindStateObject( m_cmdBuffer, XGL_STATE_BIND_VIEWPORT, m_stateViewport );
    xglCmdBindStateObject( m_cmdBuffer, XGL_STATE_BIND_COLOR_BLEND, m_colorBlend);
    xglCmdBindStateObject( m_cmdBuffer, XGL_STATE_BIND_DEPTH_STENCIL, m_stateDepthStencil );
    xglCmdBindStateObject( m_cmdBuffer, XGL_STATE_BIND_MSAA, m_stateMsaa );
}

void XglRenderFramework::GenerateClearAndPrepareBufferCmds()
{
    XGL_UINT i;

    // whatever we want to do, we do it to the whole buffer
    XGL_IMAGE_SUBRESOURCE_RANGE srRange = {};
    srRange.aspect = XGL_IMAGE_ASPECT_COLOR;
    srRange.baseMipLevel = 0;
    srRange.mipLevels = XGL_LAST_MIP_OR_SLICE;
    srRange.baseArraySlice = 0;
    srRange.arraySize = XGL_LAST_MIP_OR_SLICE;

    // clear the back buffer to dark grey
    XGL_UINT clearColor[4] = {64, 64, 64, 0};
    XGL_IMAGE_STATE_TRANSITION transitionToClear = {};
    for (i = 0; i < m_renderTargetCount; i++) {
        transitionToClear.image = m_renderTargets[i]->image();
        transitionToClear.oldState = m_renderTargets[i]->state();
        transitionToClear.newState = XGL_IMAGE_STATE_CLEAR;
        transitionToClear.subresourceRange = srRange;
        xglCmdPrepareImages( m_cmdBuffer, 1, &transitionToClear );
        m_renderTargets[i]->state(( XGL_IMAGE_STATE ) transitionToClear.newState);

        xglCmdClearColorImageRaw( m_cmdBuffer, m_renderTargets[i]->image(), clearColor, 1, &srRange );
    }

    // prepare back buffer for rendering
    XGL_IMAGE_STATE_TRANSITION transitionToRender = {};
    for (i = 0; i < m_renderTargetCount; i++) {
        transitionToRender.image = m_renderTargets[i]->image();
        transitionToRender.oldState = m_renderTargets[i]->state();
        transitionToRender.newState = XGL_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL;
        transitionToRender.subresourceRange = srRange;
        xglCmdPrepareImages( m_cmdBuffer, 1, &transitionToRender );
        m_renderTargets[i]->state(( XGL_IMAGE_STATE ) transitionToClear.newState);
    }
}

XglDescriptorSetObj::XglDescriptorSetObj(XglDevice *device)
{
    m_device = device;
    m_nextSlot = 0;

}

void XglDescriptorSetObj::AttachMemoryView(XglConstantBufferObj *constantBuffer)
{
    m_memoryViews.push_back(&constantBuffer->m_constantBufferView);
    m_memorySlots.push_back(m_nextSlot);
    m_nextSlot++;

}

void XglDescriptorSetObj::AttachSampler(XglSamplerObj *sampler)
{
    m_samplers.push_back(sampler);
    m_samplerSlots.push_back(m_nextSlot);
    m_nextSlot++;

}

void XglDescriptorSetObj::AttachImageView(XglTextureObj *texture)
{
    m_imageViews.push_back(&texture->m_textureViewInfo);
    m_imageSlots.push_back(m_nextSlot);
    m_nextSlot++;

}

XGL_DESCRIPTOR_SLOT_INFO* XglDescriptorSetObj::GetSlotInfo(vector<int>slots,
                                                           vector<XGL_DESCRIPTOR_SET_SLOT_TYPE>types,
                                                           vector<void *>objs )
{
    int nSlots = m_memorySlots.size() + m_imageSlots.size() + m_samplerSlots.size();
    m_slotInfo = (XGL_DESCRIPTOR_SLOT_INFO*) malloc( nSlots * sizeof(XGL_DESCRIPTOR_SLOT_INFO) );
    memset(m_slotInfo,0,nSlots*sizeof(XGL_DESCRIPTOR_SLOT_INFO));

    for (int i=0; i<nSlots; i++)
    {
        m_slotInfo[i].slotObjectType = XGL_SLOT_UNUSED;
    }

    for (int i=0; i<slots.size(); i++)
    {
        for (int j=0; j<m_memorySlots.size(); j++)
        {
            if ( m_memoryViews[j] == objs[i])
            {
                m_slotInfo[m_memorySlots[j]].shaderEntityIndex = slots[i];
                m_slotInfo[m_memorySlots[j]].slotObjectType = types[i];
            }
        }
        for (int j=0; j<m_imageSlots.size(); j++)
        {
            if ( m_imageViews[j] == objs[i])
            {
                m_slotInfo[m_imageSlots[j]].shaderEntityIndex = slots[i];
                m_slotInfo[m_imageSlots[j]].slotObjectType = types[i];
            }
        }
        for (int j=0; j<m_samplerSlots.size(); j++)
        {
            if ( m_samplers[j] == objs[i])
            {
                m_slotInfo[m_samplerSlots[j]].shaderEntityIndex = slots[i];
                m_slotInfo[m_samplerSlots[j]].slotObjectType = types[i];
            }
        }
    }

    // for (int i=0;i<nSlots;i++)
    // {
    //    printf("SlotInfo[%d]:  Index = %d, Type = %d\n",i,m_slotInfo[i].shaderEntityIndex, m_slotInfo[i].slotObjectType);
    //    fflush(stdout);
    // }

    return(m_slotInfo);

}
void XglDescriptorSetObj::CreateXGLDescriptorSet()
{
    init(*m_device, xgl_testing::DescriptorSet::create_info(m_nextSlot));

    begin();
    clear();

    for (int i=0; i<m_memoryViews.size();i++)
    {
        attach(m_memorySlots[i], *m_memoryViews[i]);
    }
    for (int i=0; i<m_samplers.size();i++)
    {
        attach(m_samplerSlots[i], *m_samplers[i]);
    }
    for (int i=0; i<m_imageViews.size();i++)
    {
        attach(m_imageSlots[i], *m_imageViews[i]);
    }

    end();
}

XGL_DESCRIPTOR_SET XglDescriptorSetObj::GetDescriptorSetHandle()
{
    return obj();
}

int XglDescriptorSetObj::GetTotalSlots()
{
    return m_nextSlot;
}

void XglDescriptorSetObj::BindCommandBuffer(XGL_CMD_BUFFER commandBuffer)
{
    init(*m_device, xgl_testing::DescriptorSet::create_info(m_nextSlot));

    begin();
    clear();

    for (int i=0; i<m_memoryViews.size();i++)
    {
        attach(m_memorySlots[i], *m_memoryViews[i]);
    }
    for (int i=0; i<m_samplers.size();i++)
    {
        attach(m_samplerSlots[i], *m_samplers[i]);
    }
    for (int i=0; i<m_imageViews.size();i++)
    {
        attach(m_imageSlots[i], *m_imageViews[i]);
    }

    end();

    // bind pipeline, vertex buffer (descriptor set) and WVP (dynamic memory view)
    xglCmdBindDescriptorSet(commandBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS, 0, obj(), 0 );
}

XglTextureObj::XglTextureObj(XglDevice *device)
{
    m_device = device;
    const XGL_FORMAT tex_format = { XGL_CH_FMT_B8G8R8A8, XGL_NUM_FMT_UNORM };
    const uint32_t tex_colors[2] = { 0xffff0000, 0xff00ff00 };

    memset(&m_textureViewInfo,0,sizeof(m_textureViewInfo));

    m_textureViewInfo.sType = XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO;

    const XGL_IMAGE_CREATE_INFO image = {
        .sType = XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = XGL_IMAGE_2D,
        .format = tex_format,
        .extent = { 16, 16, 1 },
        .mipLevels = 1,
        .arraySize = 1,
        .samples = 1,
        .tiling = XGL_LINEAR_TILING,
        .usage = XGL_IMAGE_USAGE_SHADER_ACCESS_READ_BIT,
        .flags = 0,
    };

    XGL_IMAGE_VIEW_CREATE_INFO view;
        view.sType = XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.pNext = NULL;
        view.image = XGL_NULL_HANDLE;
        view.viewType = XGL_IMAGE_VIEW_2D;
        view.format = image.format;
        view.channels.r = XGL_CHANNEL_SWIZZLE_R;
        view.channels.g = XGL_CHANNEL_SWIZZLE_G;
        view.channels.b = XGL_CHANNEL_SWIZZLE_B;
        view.channels.a = XGL_CHANNEL_SWIZZLE_A;
        view.subresourceRange.aspect = XGL_IMAGE_ASPECT_COLOR;
        view.subresourceRange.baseMipLevel = 0;
        view.subresourceRange.mipLevels = 1;
        view.subresourceRange.baseArraySlice = 0;
        view.subresourceRange.arraySize = 1;
        view.minLod = 0.0f;

    /* create image */
    init(*m_device, image);

    /* create image view */
    view.image = obj();
    m_textureView.init(*m_device, view);

    XGL_SUBRESOURCE_LAYOUT layout =
        subresource_layout(subresource(XGL_IMAGE_ASPECT_COLOR, 0, 0));
    m_rowPitch = layout.rowPitch;

    XGL_VOID *data;
    XGL_INT x, y;

    data = map();

    for (y = 0; y < extent().height; y++) {
        uint32_t *row = (uint32_t *) ((char *) data + layout.rowPitch * y);
        for (x = 0; x < extent().width; x++)
            row[x] = tex_colors[(x & 1) ^ (y & 1)];
    }

    unmap();

    m_textureViewInfo.view = m_textureView.obj();

}

void XglTextureObj::ChangeColors(uint32_t color1, uint32_t color2)
{
    const uint32_t tex_colors[2] = { color1, color2 };
    XGL_VOID *data;

    data = map();

    for (int y = 0; y < extent().height; y++) {
        uint32_t *row = (uint32_t *) ((char *) data + m_rowPitch * y);
        for (int x = 0; x < extent().width; x++)
            row[x] = tex_colors[(x & 1) ^ (y & 1)];
    }

    unmap();
}

XglSamplerObj::XglSamplerObj(XglDevice *device)
{
    m_device = device;

    XGL_SAMPLER_CREATE_INFO samplerCreateInfo;
    memset(&samplerCreateInfo,0,sizeof(samplerCreateInfo));
    samplerCreateInfo.sType = XGL_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter = XGL_TEX_FILTER_NEAREST;
    samplerCreateInfo.minFilter = XGL_TEX_FILTER_NEAREST;
    samplerCreateInfo.mipMode = XGL_TEX_MIPMAP_BASE;
    samplerCreateInfo.addressU = XGL_TEX_ADDRESS_WRAP;
    samplerCreateInfo.addressV = XGL_TEX_ADDRESS_WRAP;
    samplerCreateInfo.addressW = XGL_TEX_ADDRESS_WRAP;
    samplerCreateInfo.mipLodBias = 0.0;
    samplerCreateInfo.maxAnisotropy = 0.0;
    samplerCreateInfo.compareFunc = XGL_COMPARE_NEVER;
    samplerCreateInfo.minLod = 0.0;
    samplerCreateInfo.maxLod = 0.0;
    samplerCreateInfo.borderColorType = XGL_BORDER_COLOR_OPAQUE_WHITE;

    init(*m_device, samplerCreateInfo);
}

/*
 * Basic ConstantBuffer constructor. Then use create methods to fill in the details.
 */
XglConstantBufferObj::XglConstantBufferObj(XglDevice *device)
{
    m_device = device;
    m_commandBuffer = 0;

    memset(&m_constantBufferView,0,sizeof(m_constantBufferView));
}

XglConstantBufferObj::XglConstantBufferObj(XglDevice *device, int constantCount, int constantSize, const void* data)
{
    m_device = device;
    m_commandBuffer = 0;

    memset(&m_constantBufferView,0,sizeof(m_constantBufferView));
    m_numVertices = constantCount;
    m_stride = constantSize;

    const size_t allocationSize = constantCount * constantSize;
    init(*m_device, allocationSize);

    void *pData = map();
    memcpy(pData, data, allocationSize);
    unmap();

    // set up the memory view for the constant buffer
    this->m_constantBufferView.stride = 16;
    this->m_constantBufferView.range  = allocationSize;
    this->m_constantBufferView.offset = 0;
    this->m_constantBufferView.mem    = obj();
    this->m_constantBufferView.format.channelFormat = XGL_CH_FMT_R32G32B32A32;
    this->m_constantBufferView.format.numericFormat = XGL_NUM_FMT_FLOAT;
    this->m_constantBufferView.state  = XGL_MEMORY_STATE_DATA_TRANSFER;
}

void XglConstantBufferObj::Bind(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_SIZE offset, XGL_UINT binding)
{
    xglCmdBindVertexData(cmdBuffer, obj(), offset, binding);
}


void XglConstantBufferObj::SetMemoryState(XGL_MEMORY_STATE newState)
{
    XGL_RESULT err = XGL_SUCCESS;

    if (this->m_constantBufferView.state == newState)
        return;

    if (!m_commandBuffer)
    {
        m_fence.init(*m_device, xgl_testing::Fence::create_info(0));

        m_commandBuffer = new XglCommandBufferObj(m_device);

    }
    else
    {
        m_device->wait(m_fence);
    }

    // open the command buffer
    err = m_commandBuffer->BeginCommandBuffer(0);
    ASSERT_XGL_SUCCESS(err);

    XGL_MEMORY_STATE_TRANSITION transition =
        state_transition(XGL_MEMORY_STATE_DATA_TRANSFER, newState, 0, m_numVertices * m_stride);

    // write transition to the command buffer
    m_commandBuffer->PrepareMemoryRegions(1, &transition);
    this->m_constantBufferView.state = newState;

    // finish recording the command buffer
    err = m_commandBuffer->EndCommandBuffer();
    ASSERT_XGL_SUCCESS(err);

    XGL_UINT32     numMemRefs=1;
    XGL_MEMORY_REF memRefs;
    // this command buffer only uses the vertex buffer memory
    memRefs.flags = 0;
    memRefs.mem = obj();

    // submit the command buffer to the universal queue
    XGL_CMD_BUFFER bufferArray[1];
    bufferArray[0] = m_commandBuffer->GetBufferHandle();
    err = xglQueueSubmit( m_device->m_queue, 1, bufferArray, numMemRefs, &memRefs, m_fence.obj() );
    ASSERT_XGL_SUCCESS(err);
}

XglIndexBufferObj::XglIndexBufferObj(XglDevice *device)
    : XglConstantBufferObj(device)
{

}

void XglIndexBufferObj::CreateAndInitBuffer(int numIndexes, XGL_INDEX_TYPE indexType, const void* data)
{
    XGL_FORMAT viewFormat;

    m_numVertices = numIndexes;
    m_indexType = indexType;
    viewFormat.numericFormat = XGL_NUM_FMT_UINT;
    switch (indexType) {
    case XGL_INDEX_8:
        m_stride = 1;
        viewFormat.channelFormat = XGL_CH_FMT_R8;
        break;
    case XGL_INDEX_16:
        m_stride = 2;
        viewFormat.channelFormat = XGL_CH_FMT_R16;
        break;
    case XGL_INDEX_32:
        m_stride = 4;
        viewFormat.channelFormat = XGL_CH_FMT_R32;
        break;
    default:
        assert(!"unknown index type");
        break;
    }

    const size_t allocationSize = numIndexes * m_stride;
    init(*m_device, allocationSize);

    void *pData = map();
    memcpy(pData, data, allocationSize);
    unmap();

    // set up the memory view for the constant buffer
    this->m_constantBufferView.stride = m_stride;
    this->m_constantBufferView.range  = allocationSize;
    this->m_constantBufferView.offset = 0;
    this->m_constantBufferView.mem    = obj();
    this->m_constantBufferView.format.channelFormat = viewFormat.channelFormat;
    this->m_constantBufferView.format.numericFormat = viewFormat.numericFormat;
    this->m_constantBufferView.state  = XGL_MEMORY_STATE_DATA_TRANSFER;
}

void XglIndexBufferObj::Bind(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_SIZE offset)
{
    xglCmdBindIndexData(cmdBuffer, obj(), offset, m_indexType);
}

XGL_INDEX_TYPE XglIndexBufferObj::GetIndexType()
{
    return m_indexType;
}

XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* XglShaderObj::GetStageCreateInfo(XglDescriptorSetObj *descriptorSet)
{
    XGL_DESCRIPTOR_SLOT_INFO *slotInfo;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO *stageInfo = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*) calloc( 1,sizeof(XGL_PIPELINE_SHADER_STAGE_CREATE_INFO) );
    stageInfo->sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo->shader.stage = m_stage;
    stageInfo->shader.shader = obj();
    stageInfo->shader.descriptorSetMapping[0].descriptorCount = 0;
    stageInfo->shader.linkConstBufferCount = 0;
    stageInfo->shader.pLinkConstBufferInfo = XGL_NULL_HANDLE;
    stageInfo->shader.dynamicMemoryViewMapping.slotObjectType = XGL_SLOT_UNUSED;
    stageInfo->shader.dynamicMemoryViewMapping.shaderEntityIndex = 0;

    stageInfo->shader.descriptorSetMapping[0].descriptorCount = descriptorSet->GetTotalSlots();
    if (stageInfo->shader.descriptorSetMapping[0].descriptorCount)
    {
        vector<int> allSlots;
        vector<XGL_DESCRIPTOR_SET_SLOT_TYPE> allTypes;
        vector<void *> allObjs;

        allSlots.reserve(m_memSlots.size() + m_imageSlots.size() + m_samplerSlots.size());
        allTypes.reserve(m_memTypes.size() + m_imageTypes.size() + m_samplerTypes.size());
        allObjs.reserve(m_memObjs.size() + m_imageObjs.size() + m_samplerObjs.size());

        if (m_memSlots.size())
        {
            allSlots.insert(allSlots.end(), m_memSlots.begin(), m_memSlots.end());
            allTypes.insert(allTypes.end(), m_memTypes.begin(), m_memTypes.end());
            allObjs.insert(allObjs.end(), m_memObjs.begin(), m_memObjs.end());
        }
        if (m_imageSlots.size())
        {
            allSlots.insert(allSlots.end(), m_imageSlots.begin(), m_imageSlots.end());
            allTypes.insert(allTypes.end(), m_imageTypes.begin(), m_imageTypes.end());
            allObjs.insert(allObjs.end(), m_imageObjs.begin(), m_imageObjs.end());
        }
        if (m_samplerSlots.size())
        {
            allSlots.insert(allSlots.end(), m_samplerSlots.begin(), m_samplerSlots.end());
            allTypes.insert(allTypes.end(), m_samplerTypes.begin(), m_samplerTypes.end());
            allObjs.insert(allObjs.end(), m_samplerObjs.begin(), m_samplerObjs.end());
        }

         slotInfo = descriptorSet->GetSlotInfo(allSlots, allTypes, allObjs);
         stageInfo->shader.descriptorSetMapping[0].pDescriptorInfo = (const XGL_DESCRIPTOR_SLOT_INFO*) slotInfo;
    }
    return stageInfo;
}

void XglShaderObj::BindShaderEntitySlotToMemory(int slot, XGL_DESCRIPTOR_SET_SLOT_TYPE type, XglConstantBufferObj *constantBuffer)
{
    m_memSlots.push_back(slot);
    m_memTypes.push_back(type);
    m_memObjs.push_back(&constantBuffer->m_constantBufferView);

}

void XglShaderObj::BindShaderEntitySlotToImage(int slot, XGL_DESCRIPTOR_SET_SLOT_TYPE type, XglTextureObj *texture)
{
    m_imageSlots.push_back(slot);
    m_imageTypes.push_back(type);
    m_imageObjs.push_back(&texture->m_textureViewInfo);

}

void XglShaderObj::BindShaderEntitySlotToSampler(int slot, XglSamplerObj *sampler)
{
    m_samplerSlots.push_back(slot);
    m_samplerTypes.push_back(XGL_SLOT_SHADER_SAMPLER);
    m_samplerObjs.push_back(sampler);

}

XglShaderObj::XglShaderObj(XglDevice *device, const char * shader_code, XGL_PIPELINE_SHADER_STAGE stage, XglRenderFramework *framework)
{
    XGL_RESULT err = XGL_SUCCESS;
    std::vector<unsigned int> bil;
    XGL_SHADER_CREATE_INFO createInfo;
    size_t shader_len;

    m_stage = stage;
    m_device = device;

    createInfo.sType = XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    createInfo.pNext = NULL;

    if (!framework->m_use_bil) {

        shader_len = strlen(shader_code);
        createInfo.codeSize = 3 * sizeof(uint32_t) + shader_len + 1;
        createInfo.pCode = malloc(createInfo.codeSize);
        createInfo.flags = 0;

        /* try version 0 first: XGL_PIPELINE_SHADER_STAGE followed by GLSL */
        ((uint32_t *) createInfo.pCode)[0] = ICD_BIL_MAGIC;
        ((uint32_t *) createInfo.pCode)[1] = 0;
        ((uint32_t *) createInfo.pCode)[2] = stage;
        memcpy(((uint32_t *) createInfo.pCode + 3), shader_code, shader_len + 1);

        err = init_try(*m_device, createInfo);
    }

    if (framework->m_use_bil || err) {
        std::vector<unsigned int> bil;
        err = XGL_SUCCESS;

        // Use Reference GLSL to BIL compiler
        framework->GLSLtoBIL(stage, shader_code, bil);
        createInfo.pCode = bil.data();
        createInfo.codeSize = bil.size() * sizeof(unsigned int);
        createInfo.flags = 0;

        init(*m_device, createInfo);
    }
}

XglPipelineObj::XglPipelineObj(XglDevice *device)
{
    m_device = device;
    m_vi_state.attributeCount = m_vi_state.bindingCount = 0;
    m_vertexBufferCount = 0;

    m_ia_state.sType = XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO;
    m_ia_state.pNext = XGL_NULL_HANDLE;
    m_ia_state.topology = XGL_TOPOLOGY_TRIANGLE_LIST;
    m_ia_state.disableVertexReuse = XGL_FALSE;
    m_ia_state.provokingVertex = XGL_PROVOKING_VERTEX_LAST;
    m_ia_state.primitiveRestartEnable = XGL_FALSE;
    m_ia_state.primitiveRestartIndex = 0;

    m_rs_state.sType = XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO;
    m_rs_state.pNext = &m_ia_state;
    m_rs_state.depthClipEnable = XGL_FALSE;
    m_rs_state.rasterizerDiscardEnable = XGL_FALSE;
    m_rs_state.pointSize = 1.0;

    memset(&m_cb_state,0,sizeof(m_cb_state));
    m_cb_state.sType = XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO;
    m_cb_state.pNext = &m_rs_state;
    m_cb_state.alphaToCoverageEnable = XGL_FALSE;
    m_cb_state.dualSourceBlendEnable = XGL_FALSE;
    m_cb_state.logicOp = XGL_LOGIC_OP_COPY;

    m_cb_state.attachment[0].blendEnable = XGL_FALSE;
    m_cb_state.attachment[0].format.channelFormat = XGL_CH_FMT_R8G8B8A8;
    m_cb_state.attachment[0].format.numericFormat = XGL_NUM_FMT_UNORM;
    m_cb_state.attachment[0].channelWriteMask = 0xF;

    m_db_state.sType = XGL_STRUCTURE_TYPE_PIPELINE_DB_STATE_CREATE_INFO,
    m_db_state.pNext = &m_cb_state,
    m_db_state.format.channelFormat = XGL_CH_FMT_R32;
    m_db_state.format.numericFormat = XGL_NUM_FMT_DS;


};

void XglPipelineObj::AddShader(XglShaderObj* shader)
{
    m_shaderObjs.push_back(shader);
}

void XglPipelineObj::AddVertexInputAttribs(XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* vi_attrib, int count)
{
    m_vi_state.pVertexAttributeDescriptions = vi_attrib;
    m_vi_state.attributeCount = count;
}

void XglPipelineObj::AddVertexInputBindings(XGL_VERTEX_INPUT_BINDING_DESCRIPTION* vi_binding, int count)
{
    m_vi_state.pVertexBindingDescriptions = vi_binding;
    m_vi_state.bindingCount = count;
}

void XglPipelineObj::AddVertexDataBuffer(XglConstantBufferObj* vertexDataBuffer, int binding)
{
    m_vertexBufferObjs.push_back(vertexDataBuffer);
    m_vertexBufferBindings.push_back(binding);
    m_vertexBufferCount++;
}

void XglPipelineObj::SetColorAttachment(XGL_UINT binding, const XGL_PIPELINE_CB_ATTACHMENT_STATE *att)
{
    m_cb_state.attachment[binding] = *att;
}

void XglPipelineObj::CreateXGLPipeline(XglDescriptorSetObj *descriptorSet)
{
    XGL_RESULT err;
    XGL_VOID* head_ptr = &m_db_state;
    XGL_GRAPHICS_PIPELINE_CREATE_INFO info = {};

    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* shaderCreateInfo;

    for (int i=0; i<m_shaderObjs.size(); i++)
    {
        shaderCreateInfo = m_shaderObjs[i]->GetStageCreateInfo(descriptorSet);
        shaderCreateInfo->pNext = head_ptr;
        head_ptr = shaderCreateInfo;
    }

    if (m_vi_state.attributeCount && m_vi_state.bindingCount)
    {
        m_vi_state.sType = XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO;
        m_vi_state.pNext = head_ptr;
        head_ptr = &m_vi_state;
    }

    info.sType = XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.pNext = head_ptr;
    info.flags = 0;

    err = xglCreateGraphicsPipeline(m_device->device(), &info, &m_pipeline);
    assert(!err);

    err = m_device->AllocAndBindGpuMemory(m_pipeline, "Pipeline", &m_pipe_mem);
    assert(!err);
}
XGL_PIPELINE XglPipelineObj::GetPipelineHandle()
{
    return m_pipeline;
}

void XglPipelineObj::BindPipelineCommandBuffer(XGL_CMD_BUFFER m_cmdBuffer, XglDescriptorSetObj *descriptorSet)
{
    XGL_RESULT err;
    XGL_VOID* head_ptr = &m_db_state;
    XGL_GRAPHICS_PIPELINE_CREATE_INFO info = {};

    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* shaderCreateInfo;

    for (int i=0; i<m_shaderObjs.size(); i++)
    {
        shaderCreateInfo = m_shaderObjs[i]->GetStageCreateInfo(descriptorSet);
        shaderCreateInfo->pNext = head_ptr;
        head_ptr = shaderCreateInfo;
    }

    if (m_vi_state.attributeCount && m_vi_state.bindingCount)
    {
        m_vi_state.sType = XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO;
        m_vi_state.pNext = head_ptr;
        head_ptr = &m_vi_state;
    }

    info.sType = XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.pNext = head_ptr;
    info.flags = 0;

    err = xglCreateGraphicsPipeline(m_device->device(), &info, &m_pipeline);
    assert(!err);

    err = m_device->AllocAndBindGpuMemory(m_pipeline, "Pipeline", &m_pipe_mem);
    assert(!err);

    xglCmdBindPipeline( m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline );


    for (int i=0; i < m_vertexBufferCount; i++)
    {
        m_vertexBufferObjs[i]->Bind(m_cmdBuffer, m_vertexBufferObjs[i]->m_constantBufferView.offset,  m_vertexBufferBindings[i]);
    }
}

XglPipelineObj::~XglPipelineObj()
{
       if (m_pipeline != XGL_NULL_HANDLE) xglDestroyObject(m_pipeline);
}

XglMemoryRefManager::XglMemoryRefManager() {

}

void XglMemoryRefManager::AddMemoryRef(XglConstantBufferObj *constantBuffer) {
    m_bufferObjs.push_back(constantBuffer->obj());
}

void XglMemoryRefManager::AddMemoryRef(XglTextureObj *texture) {
    const std::vector<XGL_GPU_MEMORY> mems = texture->memories();
    if (!mems.empty())
        m_bufferObjs.push_back(mems[0]);
}

XGL_MEMORY_REF* XglMemoryRefManager::GetMemoryRefList() {

    XGL_MEMORY_REF *localRefs;
    XGL_UINT32     numRefs=m_bufferObjs.size();

    if (numRefs <= 0)
        return NULL;

    localRefs = (XGL_MEMORY_REF*) malloc( numRefs * sizeof(XGL_MEMORY_REF) );
    for (int i=0; i<numRefs; i++)
    {
        localRefs[i].flags = 0;
        localRefs[i].mem = m_bufferObjs[i];
    }
    return localRefs;
}
int XglMemoryRefManager::GetNumRefs() {
    return m_bufferObjs.size();
}

XglCommandBufferObj::XglCommandBufferObj(XglDevice *device)
    : xgl_testing::CmdBuffer(*device, xgl_testing::CmdBuffer::create_info(XGL_QUEUE_TYPE_GRAPHICS))
{
    m_device = device;
    m_renderTargetCount = 0;
}

XGL_CMD_BUFFER XglCommandBufferObj::GetBufferHandle()
{
    return obj();
}

XGL_RESULT XglCommandBufferObj::BeginCommandBuffer(XGL_FLAGS flags)
{
    begin(flags);
    return XGL_SUCCESS;
}

XGL_RESULT XglCommandBufferObj::EndCommandBuffer()
{
    end();
    return XGL_SUCCESS;
}

void XglCommandBufferObj::PrepareMemoryRegions(int transitionCount, XGL_MEMORY_STATE_TRANSITION *transitionPtr)
{
    xglCmdPrepareMemoryRegions(obj(), transitionCount, transitionPtr);
}

void XglCommandBufferObj::ClearAllBuffers(XGL_DEPTH_STENCIL_BIND_INFO *depthStencilBinding, XGL_IMAGE depthStencilImage)
{
    XGL_UINT i;

    // whatever we want to do, we do it to the whole buffer
    XGL_IMAGE_SUBRESOURCE_RANGE srRange = {};
    srRange.aspect = XGL_IMAGE_ASPECT_COLOR;
    srRange.baseMipLevel = 0;
    srRange.mipLevels = XGL_LAST_MIP_OR_SLICE;
    srRange.baseArraySlice = 0;
    srRange.arraySize = XGL_LAST_MIP_OR_SLICE;

    // clear the back buffer to dark grey
    XGL_UINT clearColor[4] = {64, 64, 64, 0};
    XGL_IMAGE_STATE_TRANSITION transitionToClear = {};
    for (i = 0; i < m_renderTargetCount; i++) {
        transitionToClear.image = m_renderTargets[i]->image();
        transitionToClear.oldState = m_renderTargets[i]->state();
        transitionToClear.newState = XGL_IMAGE_STATE_CLEAR;
        transitionToClear.subresourceRange = srRange;
        xglCmdPrepareImages( obj(), 1, &transitionToClear );
        m_renderTargets[i]->state(( XGL_IMAGE_STATE ) transitionToClear.newState);

        xglCmdClearColorImageRaw( obj(), m_renderTargets[i]->image(), clearColor, 1, &srRange );
    }

    if (depthStencilImage)
    {
        XGL_IMAGE_SUBRESOURCE_RANGE dsRange = {};
        dsRange.aspect = XGL_IMAGE_ASPECT_DEPTH;
        dsRange.baseMipLevel = 0;
        dsRange.mipLevels = XGL_LAST_MIP_OR_SLICE;
        dsRange.baseArraySlice = 0;
        dsRange.arraySize = XGL_LAST_MIP_OR_SLICE;

        // prepare the depth buffer for clear
        memset(&transitionToClear,0,sizeof(transitionToClear));
        transitionToClear.image = depthStencilImage;
        transitionToClear.oldState = depthStencilBinding->depthState;
        transitionToClear.newState = XGL_IMAGE_STATE_CLEAR;
        transitionToClear.subresourceRange = dsRange;
        xglCmdPrepareImages( obj(), 1, &transitionToClear );
        depthStencilBinding->depthState = transitionToClear.newState;

        xglCmdClearDepthStencil(obj(), depthStencilImage, 1.0f, 0, 1, &dsRange);

        // prepare depth buffer for rendering
        XGL_IMAGE_STATE_TRANSITION transitionToRender = {};
        transitionToRender.image = depthStencilImage;
        transitionToRender.oldState = XGL_IMAGE_STATE_CLEAR;
        transitionToRender.newState = depthStencilBinding->depthState;
        transitionToRender.subresourceRange = dsRange;
        xglCmdPrepareImages( obj(), 1, &transitionToRender );
        depthStencilBinding->depthState = transitionToClear.newState;
    }
}

void XglCommandBufferObj::BindAttachments(XGL_DEPTH_STENCIL_BIND_INFO *depthStencilBinding)
{
    XGL_UINT i;
    XGL_COLOR_ATTACHMENT_BIND_INFO  colorBindings[XGL_MAX_COLOR_ATTACHMENTS];
    XGL_IMAGE_SUBRESOURCE_RANGE srRange = {};
    srRange.aspect = XGL_IMAGE_ASPECT_COLOR;
    srRange.baseMipLevel = 0;
    srRange.mipLevels = XGL_LAST_MIP_OR_SLICE;
    srRange.baseArraySlice = 0;
    srRange.arraySize = XGL_LAST_MIP_OR_SLICE;

    XGL_IMAGE_STATE_TRANSITION transitionToRender = {};
    for(i=0; i<m_renderTargetCount; i++)
    {
        transitionToRender.image = m_renderTargets[i]->image();
        transitionToRender.oldState = m_renderTargets[i]->state();
        transitionToRender.newState = XGL_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL;
        transitionToRender.subresourceRange = srRange;
        xglCmdPrepareImages(obj(), 1, &transitionToRender );
        m_renderTargets[i]->state(( XGL_IMAGE_STATE ) transitionToRender.newState);
    }
    for (i = 0; i < m_renderTargetCount; i++) {
        colorBindings[i].view  = m_renderTargets[i]->targetView();
        colorBindings[i].colorAttachmentState = XGL_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL;
    }
    if (depthStencilBinding) {
       xglCmdBindAttachments(obj(), m_renderTargetCount, colorBindings, depthStencilBinding );
    } else {
       xglCmdBindAttachments(obj(), m_renderTargetCount, colorBindings, XGL_NULL_HANDLE );
    }
}

void XglCommandBufferObj::BindState(XGL_RASTER_STATE_OBJECT stateRaster, XGL_VIEWPORT_STATE_OBJECT stateViewport,
                                                        XGL_COLOR_BLEND_STATE_OBJECT colorBlend, XGL_DEPTH_STENCIL_STATE_OBJECT stateDepthStencil,
                                                        XGL_MSAA_STATE_OBJECT stateMsaa)
{
    // set all states
    xglCmdBindStateObject( obj(), XGL_STATE_BIND_RASTER, stateRaster );
    xglCmdBindStateObject( obj(), XGL_STATE_BIND_VIEWPORT, stateViewport );
    xglCmdBindStateObject( obj(), XGL_STATE_BIND_COLOR_BLEND, colorBlend);
    xglCmdBindStateObject( obj(), XGL_STATE_BIND_DEPTH_STENCIL, stateDepthStencil );
    xglCmdBindStateObject( obj(), XGL_STATE_BIND_MSAA, stateMsaa );
}

void XglCommandBufferObj::AddRenderTarget(XglImage *renderTarget)
{
    m_renderTargets.push_back(renderTarget);
    m_renderTargetCount++;
}

void XglCommandBufferObj::DrawIndexed(XGL_UINT firstIndex, XGL_UINT indexCount, XGL_INT vertexOffset, XGL_UINT firstInstance, XGL_UINT instanceCount)
{
    xglCmdDrawIndexed(obj(), firstIndex, indexCount, vertexOffset, firstInstance, instanceCount);
}

void XglCommandBufferObj::Draw(XGL_UINT firstVertex, XGL_UINT vertexCount, XGL_UINT firstInstance, XGL_UINT instanceCount)
{
    xglCmdDraw(obj(), firstVertex, vertexCount, firstInstance, instanceCount);
}

void XglCommandBufferObj::QueueCommandBuffer(XGL_MEMORY_REF *memRefs, XGL_UINT32 numMemRefs)
{
    XGL_RESULT err = XGL_SUCCESS;

    // submit the command buffer to the universal queue
    err = xglQueueSubmit( m_device->m_queue, 1, &obj(), numMemRefs, memRefs, NULL );
    ASSERT_XGL_SUCCESS( err );

    err = xglQueueWaitIdle( m_device->m_queue );
    ASSERT_XGL_SUCCESS( err );

    // Wait for work to finish before cleaning up.
    xglDeviceWaitIdle(m_device->device());

}
void XglCommandBufferObj::BindPipeline(XGL_PIPELINE pipeline)
{
        xglCmdBindPipeline( obj(), XGL_PIPELINE_BIND_POINT_GRAPHICS, pipeline );
}

void XglCommandBufferObj::BindDescriptorSet(XGL_DESCRIPTOR_SET descriptorSet)
{
    // bind pipeline, vertex buffer (descriptor set) and WVP (dynamic memory view)
    xglCmdBindDescriptorSet(obj(), XGL_PIPELINE_BIND_POINT_GRAPHICS, 0, descriptorSet, 0 );
}
void XglCommandBufferObj::BindIndexBuffer(XglIndexBufferObj *indexBuffer, XGL_UINT offset)
{
    xglCmdBindIndexData(obj(), indexBuffer->obj(), offset, indexBuffer->GetIndexType());
}
void XglCommandBufferObj::BindVertexBuffer(XglConstantBufferObj *vertexBuffer, XGL_UINT offset, XGL_UINT binding)
{
    xglCmdBindVertexData(obj(), vertexBuffer->obj(), offset, binding);
}

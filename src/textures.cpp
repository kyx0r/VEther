#include "textures.h"
#include "control.h"
#include "render.h"
#include "zone.h"
#include "lodepng.h"
#include "flog.h"
#include <math.h>

/* {
GVAR: logical_device -> startup.cpp
GVAR: max2DTex_size -> startup.cpp
GVAR: descriptor_pool -> control.cpp
GVAR: command_buffer -> control.cpp
GVAR: staging_buffers - > control.cpp
GVAR: current_staging_buffer -> control.cpp
GVAR: tex_dsl -> control.cpp
GVAR: number_of_swapchain_images -> swapchain.cpp
} */

VkDescriptorSet	tex_descriptor_sets[2];
static VkImage v_image[2];
static int current_tex_ds_index = 0;
static unsigned char palette[768];
static unsigned int data[256];
static VkSampler point_sampler = VK_NULL_HANDLE;

namespace textures
{

static unsigned* TexMgr8to32(unsigned char *in, int pixels, unsigned int *usepal)
{
	int i;
	unsigned *out, *data;

	out = data = (unsigned *) zone::Hunk_Alloc(pixels*4);

	for (i = 0; i < pixels; i++)
		*out++ = usepal[*in++];

	return data;
}

void InitSamplers()
{
	trace("Initializing samplers");

	VkResult err;

	if (point_sampler == VK_NULL_HANDLE)
	{
		VkSamplerCreateInfo sampler_create_info;
		memset(&sampler_create_info, 0, sizeof(sampler_create_info));
		sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler_create_info.magFilter = VK_FILTER_NEAREST;
		sampler_create_info.minFilter = VK_FILTER_NEAREST;
		sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_create_info.mipLodBias = 0.0f;
		sampler_create_info.maxAnisotropy = 1.0f;
		sampler_create_info.minLod = 0;
		sampler_create_info.maxLod = 0;

		err = vkCreateSampler(logical_device, &sampler_create_info, nullptr, &point_sampler);
		if (err != VK_SUCCESS)
			fatal("vkCreateSampler failed");


		/* 		sampler_create_info.anisotropyEnable = VK_TRUE;
				sampler_create_info.maxAnisotropy = logical_device_properties.limits.maxSamplerAnisotropy;
				err = vkCreateSampler(logical_device, &sampler_create_info, nullptr, &vulkan_globals.point_aniso_sampler);
				if (err != VK_SUCCESS)
					printf("vkCreateSampler failed");

				GL_SetObjectName((uint64_t)vulkan_globals.point_aniso_sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, "point_aniso");

				sampler_create_info.magFilter = VK_FILTER_LINEAR;
				sampler_create_info.minFilter = VK_FILTER_LINEAR;
				sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
				sampler_create_info.anisotropyEnable = VK_FALSE;
				sampler_create_info.maxAnisotropy = 1.0f;

				err = vkCreateSampler(logical_device, &sampler_create_info, nullptr, &vulkan_globals.linear_sampler);
				if (err != VK_SUCCESS)
					printf("vkCreateSampler failed");

				sampler_create_info.anisotropyEnable = VK_TRUE;
				sampler_create_info.maxAnisotropy = logical_device_properties.limits.maxSamplerAnisotropy;
				err = vkCreateSampler(logical_device, &sampler_create_info, nullptr, &vulkan_globals.linear_aniso_sampler);
				if (err != VK_SUCCESS)
					printf("vkCreateSampler failed"); */

	}
}

void TexDeinit()
{
	vkDestroySampler(logical_device, point_sampler, nullptr);
	for(int i = 0; i<current_tex_ds_index; i++)
	{
		vkDestroyImage(logical_device, v_image[i], nullptr);
		//vkDestroyImageView(logical_device, imageViews[number_of_swapchain_images+i+1], nullptr);
	}

}

void SetFilterModes(int tex_index, VkImageView *imgView)
{
	VkDescriptorImageInfo image_info;
	memset(&image_info, 0, sizeof(image_info));
	image_info.imageView = *imgView;
	image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_info.sampler = point_sampler;

	VkWriteDescriptorSet texture_write;
	memset(&texture_write, 0, sizeof(texture_write));
	texture_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	texture_write.dstSet = tex_descriptor_sets[tex_index];
	texture_write.dstBinding = 0;
	texture_write.dstArrayElement = 0;
	texture_write.descriptorCount = 1;
	texture_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	texture_write.pImageInfo = &image_info;

	vkUpdateDescriptorSets(logical_device, 1, &texture_write, 0, nullptr);
}

void GenerateColorPalette()
{
	unsigned char* dst = palette;
	for(int i = 0; i < 256; i++)
	{
		unsigned char r = 127 * (1 + sin(5 * i * 6.28318531 / 16));
		unsigned char g = 127 * (1 + sin(2 * i * 6.28318531 / 16));
		unsigned char b = 127 * (1 + sin(3 * i * 6.28318531 / 16));
		// unsigned char a = 63 * (1 + std::sin(8 * i * 6.28318531 / 16)) + 128; /*alpha channel of the palette (tRNS chunk)*/
		*dst++ = r;
		*dst++ = g;
		*dst++ = b;
		//*dst++ = a;
	}

	dst = (unsigned char*)data;
	unsigned char* src = palette;
	for (int i = 0; i < 256; i++)
	{
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = 255;
	}

}

void UploadTexture(unsigned char* image, int w, int h)
{
	VkDescriptorSetAllocateInfo dsai;
	memset(&dsai, 0, sizeof(dsai));
	dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	dsai.descriptorPool = descriptor_pool;
	dsai.descriptorSetCount = 1;
	dsai.pSetLayouts = &tex_dsl;

	vkAllocateDescriptorSets(logical_device, &dsai, &tex_descriptor_sets[current_tex_ds_index]);

	v_image[current_tex_ds_index] = render::Create2DImage(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, w, h);

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(logical_device, v_image[current_tex_ds_index], &memory_requirements);

	int mem_type = control::MemoryTypeFromProperties(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);

 try_again:;
	VkDeviceSize aligned_offset;
	vram_heap* heap = control::VramHeapDigress(memory_requirements.size, memory_requirements.alignment, &aligned_offset);	
	if(!heap)
	  {
	    if(current_tex_ds_index > 0)
	      {
		//1st allocation - OK. Do not warn.
	        warn("Failed to align the memory");
	      }
		control::VramHeapAllocate((VkDeviceSize)1073741824, mem_type);
		goto try_again;
	}
	VK_CHECK(vkBindImageMemory(logical_device, v_image[current_tex_ds_index], heap->memory, aligned_offset));
	
	//render::CreateImageViews(1, &v_image[current_tex_ds_index], VK_FORMAT_R8G8B8A8_UNORM, 0, 1);

	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	createInfo.components.r = VK_COMPONENT_SWIZZLE_R;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_A;
	createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.layerCount = 1;
	createInfo.image = v_image[current_tex_ds_index];

	VK_CHECK(vkCreateImageView(logical_device, &createInfo, 0, &imageViews[imageViewCount++]));

	SetFilterModes(current_tex_ds_index, &imageViews[imageViewCount-1]);

	//	p("%d", current_staging_buffer);
	
	unsigned char* staging_memory = control::StagingBufferDigress((w*h*4), 4);
	zone::Q_memcpy(staging_memory, image, (w * h * 4));

	VkBufferImageCopy regions = {};
	regions.bufferOffset = staging_buffers[current_staging_buffer].current_offset;
	regions.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	regions.imageSubresource.layerCount = 1;
	regions.imageSubresource.mipLevel = 0;
	regions.imageOffset = {0, 0, 0};
	regions.imageExtent.width = w;
	regions.imageExtent.height = h;
	regions.imageExtent.depth = 1;

	control::SetCommandBuffer(current_staging_buffer);

	VkImageMemoryBarrier image_memory_barrier;
	memset(&image_memory_barrier, 0, sizeof(image_memory_barrier));
	image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.image = v_image[current_tex_ds_index];
	image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_memory_barrier.subresourceRange.baseMipLevel = 0;
	image_memory_barrier.subresourceRange.levelCount = 1;
	image_memory_barrier.subresourceRange.baseArrayLayer = 0;
	image_memory_barrier.subresourceRange.layerCount = 1;

	image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	image_memory_barrier.srcAccessMask = 0;
	image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);

	vkCmdCopyBufferToImage(command_buffer, staging_buffers[current_staging_buffer].buffer, v_image[current_tex_ds_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &regions);

	image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);

	control::SetCommandBuffer(0);
	control::SubmitStagingBuffer();
	current_tex_ds_index++;

}


void FsLoadPngTexture(const char* filename)
{
	ASSERT(filename, "Null pointer passed into FsLoadPngTexture");
	// Load file and decode image.
	unsigned char mem[sizeof(std::vector<unsigned char>)];
	std::vector<unsigned char>* image = new (mem) std::vector<unsigned char>;

	unsigned width, height;
	unsigned error = lodepng::decode(*image, width, height, filename);

	if(error != 0)
	{
		fatal("Error %s : %s",error,lodepng_error_text(error));
		startup::debug_pause();
	}

	unsigned int* usepal = data;
	unsigned char* img = (unsigned char*)TexMgr8to32(image->data(), (width * height), usepal);
	UploadTexture(img, width, height);
}

bool SampleTexture()
{
	int mark = zone::Hunk_LowMark();
	//generate some image
	const unsigned w = 511;
	const unsigned h = 511;
	unsigned char* image = reinterpret_cast<unsigned char*>(zone::Hunk_Alloc(w * h));
	for(unsigned y = 0; y < h; y++)
		for(unsigned x = 0; x < w; x++)
		{
			size_t byte_index = (y * w + x);
//			printf("%d ", byte_index);
//			bool byte_half = (y * w + x) % 2 == 1;

			int color = (int)(4 * ((1 + sin(2.0 * 6.28318531 * x / (double)w))
			                       + (1 + sin(2.0 * 6.28318531 * y / (double)h))) );

			image[byte_index] |= (unsigned char)(color << (0));
		}

	unsigned int *usepal = data;
	image = (unsigned char*)TexMgr8to32(image, (w * h), usepal);

	UploadTexture(image, w, h);
	UploadTexture(image, w, h);
	zone::Hunk_FreeToLowMark(mark);

	return true;
}

} //namespace tex

#![no_main]
#![no_std]
extern crate alloc;

use alloc::{boxed::Box, vec::Vec};
use log::info;
use uefi::{
    boot::{memory_map, MemoryType, ScopedProtocol},
    fs::{FileSystem, FileSystemResult},
    mem::memory_map::{MemoryMap, MemoryMapOwned},
    prelude::*,
    proto::{
        console::gop::{GraphicsOutput, PixelFormat},
        media::fs::SimpleFileSystem,
    },
    CString16, Result,
};

use uefi::allocator::Allocator;

#[global_allocator]
static ALLOCATOR: Allocator = Allocator;

#[repr(C)]
pub struct VBE {
    pub width: usize,
    pub height: usize,
    pub pitch: usize,
    pub framebuffer: *mut u32,
    pub pixel_format: PixelFormat,
}

#[repr(C)]
pub struct MemoryMapEntry {
    pub base: u64,
    pub length: u64,
    pub ty: u32,
}

#[repr(C)]
pub struct MemoryMapRaw {
    pub entries: *const MemoryMapEntry,
    pub entry_count: usize,
}

#[repr(C)]
pub struct BootArgs {
    pub vbe: VBE,
    pub mem: MemoryMapRaw,
    pub own_size: u64,
}

pub struct MMap {
    pub boxed_entries: Box<[MemoryMapEntry]>,
    pub raw_map: MemoryMapOwned,
}

impl MMap {
    pub fn new() -> Result<Self> {
        info!("Fetching memory map");
        let raw_map = memory_map(MemoryType::LOADER_DATA)?;
        info!("Mapping entries");
        let mut vec: Vec<MemoryMapEntry> = Vec::new();
        for desc in raw_map.entries() {
            let mem_type = MemoryType(desc.ty.0);

            match mem_type {
                MemoryType::CONVENTIONAL
                | MemoryType::BOOT_SERVICES_CODE
                | MemoryType::BOOT_SERVICES_DATA
                | MemoryType::LOADER_DATA
                | MemoryType::LOADER_CODE => {
                    vec.push(MemoryMapEntry {
                        base: desc.phys_start,
                        length: desc.page_count,
                        ty: desc.ty.0 as u32,
                    });
                }
                _ => {}
            }
        }

        let boxed = vec.into_boxed_slice();
        info!("Creating memory object");
        Ok(MMap {
            boxed_entries: boxed,
            raw_map,
        })
    }

    pub fn as_raw(&self) -> MemoryMapRaw {
        MemoryMapRaw {
            entries: self.boxed_entries.as_ptr(),
            entry_count: self.boxed_entries.len(),
        }
    }
}

impl VBE {
    pub fn new() -> Result<Self> {
        info!("Getting GOP handle");
        let gop_handle = uefi::boot::get_handle_for_protocol::<GraphicsOutput>()?;

        info!("Opening GOP protocol");
        let mut gop = unsafe {
            boot::open_protocol::<GraphicsOutput>(
                boot::OpenProtocolParams {
                    handle: gop_handle,
                    agent: boot::image_handle(),
                    controller: None,
                },
                boot::OpenProtocolAttributes::GetProtocol,
            )?
        };

        info!("Getting mode info");
        let mode_info = gop.current_mode_info();

        info!("Getting framebuffer");
        let fb = gop.frame_buffer().as_mut_ptr() as *mut u32;

        info!("Getting pixel format");
        let pixel_format = mode_info.pixel_format();

        let pitch_pixels = mode_info.stride();

        info!("VBE struct created");
        Ok(Self {
            width: mode_info.resolution().0 as usize,
            height: mode_info.resolution().1 as usize,
            pitch: pitch_pixels,
            framebuffer: fb,
            pixel_format,
        })
    }
}

fn read_file(path: &str) -> FileSystemResult<Vec<u8>> {
    let path: CString16 = CString16::try_from(path).expect("Failed to parse file path");
    let fs: ScopedProtocol<SimpleFileSystem> =
        boot::get_image_file_system(boot::image_handle()).expect("Failed to get filesystem image");
    let mut fs = FileSystem::new(fs);
    fs.read(path.as_ref())
}

#[entry]
fn main() -> Status {
    uefi::helpers::init().unwrap();

    system::with_stdout(|stdout| {
        stdout.clear().unwrap();
    });

    info!("UEFI Bootloader started");
    info!("Initializing graphics");
    let vbe = VBE::new().unwrap();
    info!("GOP initialized: {}x{}", vbe.width, vbe.height);

    info!("Getting memory info");
    let mmap = MMap::new().unwrap();
    let mem = mmap.as_raw();
    info!("Memory info found! Entries found: {}", mem.entry_count);

    info!("Loading kernel from /kernel.bin");
    let kernel_data = read_file("\\kernel.bin").unwrap();
    info!("Kernel loaded: {} bytes", kernel_data.len());

    const KERNEL_BASE: u64 = 0x100000;

    info!("Allocating memory at 0x{:X}", KERNEL_BASE);
    let page_count = (kernel_data.len() + 4095) / 4096;
    let kernel_addr = boot::allocate_pages(
        boot::AllocateType::Address(KERNEL_BASE),
        MemoryType::LOADER_CODE,
        page_count,
    )
    .expect("Failed to allocate memory for kernel");
    info!("Allocated {} pages", page_count);

    info!("Copying kernel to memory");
    let kernel_dest = unsafe {
        core::slice::from_raw_parts_mut(kernel_addr.as_ptr() as *mut u8, kernel_data.len())
    };
    kernel_dest.copy_from_slice(&kernel_data);
    info!("Kernel copied successfully");

    let boot = BootArgs {
        vbe,
        mem,
        own_size: kernel_data.len() as u64,
    };

    info!("Exiting boot services");
    unsafe {
        let _ = boot::exit_boot_services(Some(boot::MemoryType::LOADER_DATA));
    }

    let boot_ptr = &boot as *const BootArgs;
    unsafe {
        core::arch::asm!(
            "call {entry}",
            entry = in(reg) kernel_addr.as_ptr(),
            in("rdi") boot_ptr,
            options(noreturn)
        );
    }
}
